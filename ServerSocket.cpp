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
handleError()
{
    /* the POSIX recvmsg() has top be used to know the error type. */
    char iobuffer[UIO_MAXIOV];
    struct iovec dummy_iovec = {.iov_base = iobuffer, .iov_len = UIO_MAXIOV };
    struct msghdr msg_hdr;
    msg_hdr.msg_iov = &dummy_iovec;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_name = nullptr;     // because not an UDP stream
    msg_hdr.msg_namelen = 0;        // because not an UDP stream
    msg_hdr.msg_control = nullptr;
    msg_hdr.msg_controllen = 0;
    
    ssize_t err = ::recvmsg(socket_descr, &msg_hdr, MSG_PEEK );
    
    // an error is expected on that socket, so -1 is expected.
    assert(err == -1);
    
    return _getErrorType(errno);// usual errno.
}

MsgSocket::ErrorType 
ServerSocket::
_getErrorType(int err) 
{
    MsgSocket::ErrorType err_type;
    
    switch (err)
    {
        default:     
            // default rtreatment.
            err_type =  MsgSocket::_getErrorType(err);
            break;
    }

    return err_type;
}

