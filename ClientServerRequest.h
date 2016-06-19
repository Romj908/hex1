/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientServerRequest.h
 * Author: jlaclavere
 *
 * Created on May 8, 2016, 10:24 PM
 */

#ifndef CLIENTSERVERREQUEST_H
#define CLIENTSERVERREQUEST_H

#include "assert.h"

#include <memory>

#include "util/ipUtilities.h"
#include "ClientServerPrtcl.h"


typedef std::shared_ptr<ClientServerL1Msg> ClientServerL1MsgPtr;
typedef std::unique_ptr<ClientServerL1Msg> ClientServerL1MsgUPtr;

typedef std::shared_ptr<ClientServerMsgBody> ClientServerMsgBodyPtr;
typedef std::unique_ptr<ClientServerMsgBody> ClientServerMsgBodyUPtr;


/*******************************************************************************
 * 
 * ClientServerRequest : A upper level request to send some data to the peer.
 * 
 *******************************************************************************/
class ClientServerRequest
{
protected:
    long l2_payload_length;      // total number of l2 bytes to send. Several l1 messages may be necessary.
    
    long nb_payload_bytes_sent;  // number of bytes currently sent.
    
    // real constructor - it's protected
    ClientServerRequest(int number_of_bytes) 
    : l2_payload_length(number_of_bytes) 
    {
        nb_payload_bytes_sent = 0;
    };
    virtual ~ClientServerRequest() = default;
    
private:
    // forbid copy.
    ClientServerRequest(const ClientServerRequest& orig) = delete;
    
public:
    bool 
    someDataToSend() const {return l2_payload_length > nb_payload_bytes_sent;}
    
    int  
    getPayloadLength() const {return l2_payload_length; }
    
    virtual ClientServerMsgBodyPtr 
    buildNextL2Msg(ClientServerL1MessageId &l1_msg_id, int &length) = 0;
    
    
};

typedef std::shared_ptr<ClientServerRequest> ClientServerRequestPtr;

typedef std::unique_ptr<ClientServerRequest> ClientServerRequestUPtr;

/*******************************************************************************
 * 
 * ClientServerMsgRequest : The request is for a single message
 * 
 *******************************************************************************/
class ClientServerMsgRequest : public ClientServerRequest
{
    ClientServerL1MessageId l1_msg_id;  // l1 message id of the next message.
    
    ClientServerMsgBodyPtr  msg_ptr;
    
public:
    ClientServerMsgRequest(ClientServerMsgBodyPtr  l2_msg_ptr, 
                           int nb_bytes,
                           ClientServerL1MessageId msg_id = ClientServerL1MessageId::DATA_TRANSFER_REQ)
    : ClientServerRequest(nb_bytes),  msg_ptr(l2_msg_ptr), l1_msg_id(msg_id) 
    {
        assert(nb_bytes < CLIENTSERVER_MAX_L1_MSG_BODY_SIZE);
    }
        
    virtual ~ClientServerMsgRequest() = default;
    
private:
    ClientServerMsgRequest(const ClientServerMsgRequest& orig) = delete;

protected:
    virtual ClientServerMsgBodyPtr 
    buildNextL2Msg(ClientServerL1MessageId &l1_msg_id, int &length) override ;
     
    
};

typedef std::shared_ptr<ClientServerMsgRequest> ClientServerMsgRequestPtr;

typedef std::unique_ptr<ClientServerMsgRequest> ClientServerMsgRequestUPtr;


#endif /* CLIENTSERVERREQUEST_H */

