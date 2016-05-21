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

/* Segment the socket flow in messages having a reasonnable size */
#define CLIENTSERVER_MAX_L2_MSG_SIZE  (1024*8)

/* some constants */
#define CLIENTSERVER_VERSION_STRING_LENGTH  32
#define CLIENTSERVER_USER_STRING_LENGTH  32
#define CLIENTSERVER_USER_PASSWD_STRING_LENGTH  32

#define CLIENTSERVER_FILE_NAME_LENGTH  256

enum class ClientServerL1MessageId 
: unsigned short
{
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
    char client_version[CLIENTSERVER_VERSION_STRING_LENGTH];
};

struct ConnectionCnf
{
    enum Cyphering { NO_CICPHERING };
    
    Cyphering       ciphering;
    char            key[128];
};

struct ConnectionRej
{
    enum Cause { IP_ADDRESS_CONFLICT, BAD_CLIENT_VERSION};
    
    Cause       cause; 
    char        server_sw_version[CLIENTSERVER_VERSION_STRING_LENGTH];
};



struct RegistrationReq
{
    char user_name[CLIENTSERVER_VERSION_STRING_LENGTH];
    char password[CLIENTSERVER_USER_PASSWD_STRING_LENGTH];
};

struct RegistrationCnf
{
    
};

struct RegistrationRej
{
    enum Cause { USER_ALREADY_EXISTING };
    Cause cause; 
};

enum class TransferDirection : unsigned char {FROM_SERVER, FROM_CLIENT};

typedef long DataTransferId;

struct FileTransferReqHeader
{
    
    DataTransferId transfer_id; // global transaction identifier

    unsigned long  file_size;  // total file's size in number of bytes
    
    unsigned short      l2_action; // for private use by the L2
        
    char file_name[CLIENTSERVER_FILE_NAME_LENGTH]; // 
};

struct FileTransferReq
{
    FileTransferReqHeader header;
    char  l2_payload [CLIENTSERVER_MAX_L2_MSG_SIZE - sizeof(header)];
};


struct DataTransferReqHeader
{    
    DataTransferId transfer_id; // global transaction identifier
    unsigned long  total_size;  // in number of bytes
    
    unsigned short      l2_action; // for private use by the L2        
};

struct DataTransferReq
{
    DataTransferReqHeader header;
    char  l2_payload [CLIENTSERVER_MAX_L2_MSG_SIZE - sizeof(header)];
};


struct DataTransferHeader
{
    DataTransferId  transfer_id; // per client : transaction identifier
    unsigned long  byte_position; // offest in the file of the first data byte
};

struct DataTransferBlk
{
    DataTransferHeader  header;
    char  l2_payload [CLIENTSERVER_MAX_L2_MSG_SIZE - sizeof(header)];
};

typedef char L1Payload[CLIENTSERVER_MAX_L2_MSG_SIZE];

union ClientServerMsg
{
    L1Payload     l1_payload;
    
    ConnectionReq connection_req;
    ConnectionCnf connection_cnf;
    ConnectionRej connection_rej;
    
    RegistrationReq registration_req;
    RegistrationCnf registration_cnf;
    RegistrationRej registration_rej;
    
    DataTransferReq     data_transfer_req;
    DataTransferBlk     data_transfer_blk;
    
    FileTransferReq     file_transfer_req;

    
};

struct ClientServerL1MsgHeader
{
    unsigned long                  lenght; // allow for >64KBytes
    union {
        unsigned short                  u16;  
        ClientServerL1MessageId         l1_msg_id;
    } id;
    TransferDirection               from;
    unsigned char                   _notyetused; // padding
    
    // ciphering information around here...
    
};

struct ClientServerL1Msg
{
    ClientServerL1MsgHeader        l1_header;
    ClientServerMsg                body;
};


#endif /* CLIENTSERVERPRTCL_H */

