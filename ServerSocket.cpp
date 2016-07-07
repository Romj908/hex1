/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerSocket.cpp
 * Author: jlaclavere
 * 
 * Created on June 6, 2016, 11:07 PM
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <unistd.h>// getpid()
#include <assert.h>
#include "util/ipUtilities.h"

#include "hex1Protocol.h"

#include "ServerSocket.h"

ServerSocket::
ServerSocket(const sockaddr_in& serverAddr, int socket_desc)
        : MsgSocket(serverAddr, socket_desc, TransferDirection::FROM_SERVER)
{    
    this->socketConfiguration();
}


ServerSocket::
~ServerSocket() 
{
}


MsgSocket::ErrorType 
ServerSocket::
_getRxErrorType(int err) 
{
    MsgSocket::ErrorType err_type;
    
    switch (err)
    {
        default:     
            // default rtreatment.
            err_type =  MsgSocket::_getRxErrorType(err);
            break;
    }

    return err_type;
}

