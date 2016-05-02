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

/* Segment the sokect flux in messages having a reasonnable size */
#define CLIENTSERVER_SOCK_MAX_MSG_SIZE  ((1024*32)- (sizeof(ClientServerMsg) - sizeof(ClientReqMsgBody))



enum class ClientServerMessageId 
: unsigned short
{
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
    
    DATA_REQ,
    DATA_CNF,
            
};


union ClientReqMsgBody
{
    
};

struct ClientServerMsg
{
    ClientServerMessageId           id;
    unsigned short                  lenght; 
    
    ClientReqMsgBody                body;
};




enum class ServerCnfId 
: unsigned short
{
    
};

union ServerCnfMsgBody
{
    
};

struct ServerCnfMsg
{
    ServerCnfId         id;
    unsigned short      lenght;
    
    ServerCnfMsgBody    body;
};

 

#endif /* CLIENTSERVERPRTCL_H */

