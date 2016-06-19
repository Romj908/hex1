/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerSocket.h
 * Author: jlaclavere
 *
 * Created on June 6, 2016, 11:07 PM
 */

#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H
#include "MsgSocket.h"

/*
 * 
 * ServerSocket : socket handling of a client's socket, on server side.
 * (it's not the server's socket listening for new connections!)
 * 
 * 
 */
class ServerSocket : public MsgSocket
{
    ServerSocket(const ServerSocket& orig) = delete;
public:
    ServerSocket(const sockaddr_in &serverAddr, int sock_descr);
    virtual ~ServerSocket();
        
    virtual MsgSocket::ErrorType 
    handleError() override;

    virtual MsgSocket::ErrorType 
    _getErrorType(int sock_err) override;
    // methods specific to the server mode.

};

#endif /* SERVERSOCKET_H */

