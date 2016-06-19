/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientSocket.cpp
 * Author: jlaclavere
 * 
 * Created on June 6, 2016, 11:26 PM
 */

#include <cstdlib>
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>// getpid()
#include <assert.h>
#include "util/ipUtilities.h"

#include "hex1Protocol.h"

#include "ClientSocket.h"

ClientSocket::
ClientSocket(const sockaddr_in& serverAddr, int socket_desc)
        : MsgSocket(serverAddr, socket_desc, TransferDirection::FROM_CLIENT)
{    
    this->socketConfiguration();
    this->connect();
}

ClientSocket::
~ClientSocket() 
{
}


void
ClientSocket::
connect()
{
    assert(sock_state == State::SOCK_NULL);
    assert(from == TransferDirection::FROM_CLIENT);
    
    std::cout << "Client starting connection request...";
    
    sock_state = State::SOCK_CONNECTING;
    
    // start an asynchronous connection.
    int status = ::connect( socket_descr, (struct sockaddr *) &peer_ip_addr, sizeof(peer_ip_addr));
    
    if (status < 0)
    {
        if (status != EWOULDBLOCK && status != EINTR)
            throw("client socket's connect() error");
    }
}

MsgSocket::ErrorType 
ClientSocket::
_getErrorType(int err) 
{
    MsgSocket::ErrorType err_type;
    
    switch (err)
    {
        case EINTR:
            /*
                signal was received while blocked in the the accept() function (ansynchronous connection continuing) 
            */
            if (sock_state == MsgSocket::State::SOCK_CONNECTING)
            {
                err_type = MsgSocket::ErrorType::IGNORE;
            }
            else
                err_type =  MsgSocket::_getErrorType(err);
            break;
                
        default:     
            // default rtreatment.
            err_type =  MsgSocket::_getErrorType(err);
            break;
    }

    return err_type;
}


MsgSocket::ErrorType 
ClientSocket::
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


bool 
ClientSocket::
pool()
{
    bool incoming_msg = false;
    
    struct pollfd fds;
    fds.fd = this->socket_descr;
    /*
     * 
        POLLIN
            Data other than high-priority data may be read without blocking.

            For STREAMS, this flag is set in revents even if the message is of zero length. This flag shall be equivalent to POLLRDNORM | POLLRDBAND.

        POLLRDNORM
            Normal data may be read without blocking.

            For STREAMS, data on priority band 0 may be read without blocking. This flag is set in revents even if the message is of zero length.

        POLLRDBAND
            Priority data may be read without blocking.

            For STREAMS, data on priority bands greater than 0 may be read without blocking. This flag is set in revents even if the message is of zero length.

        POLLPRI
            High-priority data may be read without blocking.

            For STREAMS, this flag is set in revents even if the message is of zero length.

        POLLOUT
            Normal data may be written without blocking.

            For STREAMS, data on priority band 0 may be written without blocking.

        POLLWRNORM
            Equivalent to POLLOUT. 
     
        POLLWRBAND
            Priority data may be written.

            For STREAMS, data on priority bands greater than 0 may be written without blocking. If any priority band has been written to on this STREAM, this event only examines bands that have been written to at least once.

        POLLERR
            An error has occurred on the device or stream. This flag is only valid in the revents bitmask; it shall be ignored in the events member. 
        POLLHUP
            The device has been disconnected. This event and POLLOUT are mutually-exclusive; a stream can never be writable if a hangup has occurred. However, this event and POLLIN, POLLRDNORM, POLLRDBAND, or POLLPRI are not mutually-exclusive. This flag is only valid in the revents bitmask; it shall be ignored in the events member. 
        POLLNVAL
            The specified fd value is invalid. This flag is only valid in the revents member; it shall ignored in the events member.     
     */    
    
    // always check if new data have just arrived.
    fds.events = POLLRDNORM ;
    
    if (!tx_fifo.empty() || sock_state == MsgSocket::State::SOCK_CONNECTING)
    {
        // we need to know if the socket is ready to send some data.
        fds.events |= POLLWRNORM ;        
    }
    //int poll(struct pollfd fds[], nfds_t nfds, int timeout); 
    int stat = ::poll(&fds, 1, 0 /*shall return immediately*/);
    
    if (stat < 0)
    {
        // some abnormal situation happened, and the cause is to read in errno.
        /*
            EAGAIN
                The allocation of internal data structures failed but a subsequent request may succeed. 
            EINTR
                A signal was caught during poll(). 
            EINVAL
                The nfds argument is greater than {OPEN_MAX}, or one of the fd members refers to a 
                STREAM or multiplexer that is linked (directly or indirectly) downstream from a multiplexer.*/
        switch(errno)
        {
            case EAGAIN:
            case EINTR:
                return incoming_msg;
            default:
                assert(0);
        }
    }
    else if (stat == 0)
    {
        // nothing in revent. So nothing to do.
        return incoming_msg;
    }
    else if (fds.revents & POLLERR)
    {
        handleError();
    }
    else if (fds.revents & POLLHUP)
    {
        sock_state = MsgSocket::State::SOCK_DISCONNECTED;
        throw peer_disconnection{"socket disconnection (POLLHUP)"};
    }
    else 
    {
        // normal events.
        if (fds.revents & POLLRDNORM)
        {
            incoming_msg = recvNextData();
        }
        if (fds.revents & POLLWRNORM)
        {
            if (sock_state == MsgSocket::State::SOCK_CONNECTING)
            {
                /*
                 A file descriptor for a socket that is listening for connections shall indicate 
                 * that it is ready for reading, once connections are available. 
                 * A file descriptor for a socket that is connecting asynchronously shall indicate 
                 * that it is ready for writing, once a connection has been established. * 
                 */
                sock_state = MsgSocket::State::SOCK_CONNECTED;
            }
            
            sendNextData();
        }
    }
    return incoming_msg;
    
}
