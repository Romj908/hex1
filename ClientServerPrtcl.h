/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientServerPrtcl.h
 * Author: jlaclavere
 *
 * Created on April 13, 2016, 8:21 PM
 */

#ifndef CLIENTSERVERPRTCL_H
#define CLIENTSERVERPRTCL_H

#include <cstdint>   // int16_t, etc...

/* Segment the socket flow in messages having a reasonnable size */
#define CLIENTSERVER_MAX_L1_MSG_BODY_SIZE  (1024*8)

/* some constants */
#define CLIENTSERVER_VERSION_STRING_LENGTH  32
#define CLIENTSERVER_USER_STRING_LENGTH  32
#define CLIENTSERVER_USER_PASSWD_STRING_LENGTH  32
#define CLIENTSERVER_USER_MAIL_STRING_LENGTH  128

#define CLIENTSERVER_FILE_NAME_LENGTH  256


#define CLIENTSERVER_CIPHERING_KEY_LENGTH  128   /* to_do*/

using CipheringKeyData = char[CLIENTSERVER_CIPHERING_KEY_LENGTH];

inline void cipheringKeyDataCopy(CipheringKeyData& to, const CipheringKeyData& from)
{
    const char *p1 = reinterpret_cast<const char *>(&from);
    const char *p1e = p1+CLIENTSERVER_CIPHERING_KEY_LENGTH;
    char *p2 = reinterpret_cast<char *>(&to);
    
    do 
    {
        *p2++ = *p1++;
    } while (p1 != p1e);
}

/* 
 * type of the field carrying the size of a message
 * 2 bytes would be enough for the initial implementation where segment data is segmented into short messages. 
 * Now, in order to not prevent different implementations of the file transfer, 4 bytes are better.
 * allow for >64KBytes, but >2G bytes not supported. 
 */
using ClientServerLength_t = int32_t; 

using ClientServerIdentity_t = int16_t;

enum class TransferDirection 
: unsigned char 
{
    FROM_SERVER, 
    FROM_CLIENT
};


enum class ClientServerL1MessageId 
: ClientServerIdentity_t
{
    NONE = 0,
    /* L1 messages */
    CONNECTION_REQ,
    CONNECTION_CNF,
    CONNECTION_REJ,
            
    LOGGING_REQ,
    LOGGING_CNF,
    LOGGING_REJ,
            
    REGISTRATE_REQ,
    REGISTRATE_CNF, 
    REGISTRATE_REJ,
            
    DISCONNECT_REQ,
    DISCONNECT_CNF,
    DISCONNECT_IND,
            
    POLL_REQ,
    POLL_CNF,
    
    /* L2 messages encapsulation in L1 data messages. */
    /* 1) for simple user actions */
    DATA_TRANSFER_REQ,
    TRANSFER_DATA_BLK,      // subsequent blocks
    
    /* 2) for file download/upload - these messages are used in both directions */
    FILE_TRANSFER_REQ, // First block of the streaming (description)
    
    
};

struct ConnectionReq
{
    char client_version[CLIENTSERVER_VERSION_STRING_LENGTH];// null terminated string (last significant character is \0)
};

struct ConnectionCnf
{
    enum Cyphering { NO_CICPHERING, CIPHERING_A };
    
    Cyphering       ciphering;
    CipheringKeyData key;
};

struct ConnectionRej
{
    enum Cause { NO_CAUSE, IP_ADDRESS_CONFLICT, BAD_CLIENT_VERSION, UNEXPECTED_CONN_REQ};
    
    Cause       cause; 
    char        server_sw_version[CLIENTSERVER_VERSION_STRING_LENGTH];
};

struct UserLoggingReq
{
    char user_name[CLIENTSERVER_VERSION_STRING_LENGTH];
    char password[CLIENTSERVER_USER_PASSWD_STRING_LENGTH];
};
struct UserLoggingRej
{
    enum Cause { NO_CAUSE, USER_UNKNOWN, WRONG_PASSWORD };
    Cause cause; 
};

struct UserLoggingCnf
{
};

struct RegistrationReq
{
    char user_name[CLIENTSERVER_VERSION_STRING_LENGTH]; // null terminated string (last significant character is \0)
    char password[CLIENTSERVER_USER_PASSWD_STRING_LENGTH];// null terminated string (last significant character is \0)
    char mail_address[CLIENTSERVER_USER_MAIL_STRING_LENGTH];// null terminated string (last significant character is \0)
};

struct RegistrationCnf
{
    
};

struct RegistrationRej
{
    enum Cause { NO_CAUSE, USER_ALREADY_EXISTING, INCORRECT_EMAIL, UNEXPECTED_REGISTRATION };
    Cause cause; 
};

typedef long DataTransferId;

struct FileTransferReqHeader
{
    
    size_t  file_size;  // total file's size in number of bytes
    
    DataTransferId transfer_id; // global transaction identifier

    
    int16_t l2_action; // for private use by the L2, tbd to_do
        
    char file_name[CLIENTSERVER_FILE_NAME_LENGTH]; // 
};

struct FileTransferReq
{
    FileTransferReqHeader header;
    char  l2_payload [CLIENTSERVER_MAX_L1_MSG_BODY_SIZE - sizeof(header)];
};


struct DataTransferReqHeader
{    
    DataTransferId transfer_id; // global transaction identifier
    size_t      total_size;  // in number of bytes
    int16_t      l2_action; // for private use by the L2        
};

struct DataTransferReq
{
    DataTransferReqHeader header;
    char  l2_payload [CLIENTSERVER_MAX_L1_MSG_BODY_SIZE - sizeof(header)];
};


struct DataTransferHeader
{
    DataTransferId  transfer_id; // per client : transaction identifier
    int32_t  byte_position; // offest in the file of the first data byte
};

struct DataTransferBlk
{
    DataTransferHeader  header;
    char  l2_payload [CLIENTSERVER_MAX_L1_MSG_BODY_SIZE - sizeof(header)];
};

typedef char L1Payload[CLIENTSERVER_MAX_L1_MSG_BODY_SIZE];

union ClientServerMsgBody
{
    L1Payload     l1_payload;
    
    ConnectionReq connection_req;
    ConnectionCnf connection_cnf;
    ConnectionRej connection_rej;
    
    RegistrationReq registration_req;
    RegistrationCnf registration_cnf;
    RegistrationRej registration_rej;
    
    UserLoggingReq user_logging_req;
    UserLoggingCnf user_logging_cnf;
    UserLoggingRej user_logging_rej;
    
    DataTransferReq     data_transfer_req;
    DataTransferBlk     data_transfer_blk;
    
    FileTransferReq     file_transfer_req;

    
};

struct ClientServerL1MsgHeader
{
    ClientServerLength_t                lenght; 
    union {
        ClientServerIdentity_t          val;  
        ClientServerL1MessageId         l1_msg_id;
    } id;
    TransferDirection               from;
    unsigned char                   _notyetused; // padding
    
    // ciphering information around here...
    
};

struct ClientServerL1Msg
{
    ClientServerL1MsgHeader        l1_header;
    ClientServerMsgBody            l1_body;
};


#endif /* CLIENTSERVERPRTCL_H */

