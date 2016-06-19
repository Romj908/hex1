/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientSocket.h
 * Author: jlaclavere
 *
 * Created on June 6, 2016, 11:26 PM
 */

#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H
#include "MsgSocket.h"

/*
 * 
 * ClientSocket : socket handling of a client's socket, on client side.
 * 
 */
class ClientSocket : public MsgSocket
{
    ClientSocket() = delete;
    ClientSocket(const ClientSocket& orig) = delete;
    
public:
     
    ClientSocket(const sockaddr_in &serverAddr, int sock_descr);
    virtual ~ClientSocket();
    
    virtual MsgSocket::ErrorType 
    handleError() override;

    virtual MsgSocket::ErrorType 
    _getErrorType(int sock_err) override;
    
    // methods specific to the client mode.
    void connect();
    
    /* 
     * check whether some data can be sent or if some new data have been received 
     * Returns whether new message(s) have been received 
     */
    bool pool();
    
    
};


#endif /* CLIENTSOCKET_H */

