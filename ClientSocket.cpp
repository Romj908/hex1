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

#include <libexplain/gcc_attributes.h>

#include <libexplain/connect.h>

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
    assert(from == TransferDirection::FROM_CLIENT);
    
    std::cout << "\nClient starting connection request...";
    
    sock_state = State::SOCK_CONNECTING;
    
    // start an asynchronous connection.
    int status = ::connect( socket_descr, (struct sockaddr *) &peer_ip_addr, sizeof(peer_ip_addr));
    
    if (status < 0)
    {
        assert(status == -1);
        int err = errno;
        /*
        EADDRNOTAVAIL
              The specified address is not available from the local machine.

       EAFNOSUPPORT
              The specified address is not a valid address for the address
              family of the specified socket.

       EALREADY
              A connection request is already in progress for the specified
              socket.

       EBADF  The socket argument is not a valid file descriptor.

       ECONNREFUSED
              The target address was not listening for connections or
              refused the connection request.

       EINPROGRESS
              O_NONBLOCK is set for the file descriptor for the socket and
              the connection cannot be immediately established; the
              connection shall be established asynchronously.

       EINTR  The attempt to establish a connection was interrupted by
              delivery of a signal that was caught; the connection shall be
              established asynchronously.

       EISCONN
              The specified socket is connection-mode and is already
              connected.

       ENETUNREACH
              No route to the network is present.

       ENOTSOCK
              The socket argument does not refer to a socket.

       EPROTOTYPE
              The specified address has a different type than the socket
              bound to the specified peer address.

       ETIMEDOUT
              The attempt to connect timed out before a connection was made.

       If the address family of the socket is AF_UNIX, then connect() shall
       fail if:

       EIO    An I/O error occurred while reading from or writing to the
              file system.

       ELOOP  A loop exists in symbolic links encountered during resolution
              of the pathname in address.

       ENAMETOOLONG
              The length of a component of a pathname is longer than
              {NAME_MAX}.

       ENOENT A component of the pathname does not name an existing file or
              the pathname is an empty string.

       ENOTDIR
              A component of the path prefix of the pathname in address
              names an existing file that is neither a directory nor a
              symbolic link to a directory, or the pathname in address
              contains at least one non-<slash> character and ends with one
              or more trailing <slash> characters and the last pathname
              component names an existing file that is neither a directory
              nor a symbolic link to a directory.

       The connect() function may fail if:

       EACCES Search permission is denied for a component of the path
              prefix; or write access to the named socket is denied.

       EADDRINUSE
              Attempt to establish a connection that uses addresses that are
              already in use.

       ECONNRESET
              Remote host reset the connection request.

       EHOSTUNREACH
              The destination host cannot be reached (probably because the
              host is down or a remote router cannot reach it).

       EINVAL The address_len argument is not a valid length for the address
              family; or invalid address family in the sockaddr structure.

       ELOOP  More than {SYMLOOP_MAX} symbolic links were encountered during
              resolution of the pathname in address.

       ENAMETOOLONG
              The length of a pathname exceeds {PATH_MAX}, or pathname
              resolution of a symbolic link produced an intermediate result
              with a length that exceeds {PATH_MAX}.

       ENETDOWN
              The local network interface used to reach the destination is
              down.

       ENOBUFS
              No buffer space is available.

       EOPNOTSUPP
              The socket is listening and cannot be connected.

         */
        switch (err)
        {
            case EINPROGRESS:
                // normal case
                break;
                
            case EINTR:
                // normal even if unusual with non-blocking socket
                break;
                        
            case ENOBUFS:   // system ressource issue
            case ENETDOWN:  // no ethernet connection
            case EHOSTUNREACH: // server is not running
            case ECONNRESET:   // server is shutting down
            case ECONNREFUSED: // server's firewall may be denying to use that port.
            {
                char message[3000];
                explain_message_errno_connect(message, sizeof(message), 
                                            err, 
                                            socket_descr, 
                                            (struct sockaddr *) &peer_ip_addr, 
                                            sizeof(peer_ip_addr));
                std::cerr << std::endl << message << std::endl;
                throw "\n::connect() recoverable error ";                
            }

            case EAFNOSUPPORT: // bug
            case EBADF:     // bug
            case EPROTOTYPE:  // bug
            case ENOTSOCK:    // bug
            case EINVAL:    // bug
            case EOPNOTSUPP:   // only server's socket is listening => bug
            case EALREADY:  // should be prevented by state machine => bug
            case EISCONN:   // should be prevented by state machine => bug
                std::cerr << "\n::connect() bug, error " << err << std::endl;
                assert(0);  // 
                break;
                
            case EADDRINUSE: // to clarify
            case EADDRNOTAVAIL: // to clarify
            case ETIMEDOUT:     // to clarify
                std::cerr << "\n::connect() unexpected error " << err << std::endl;
                std::cerr.flush();
                assert(0);  // 
                break;
                
            // Errors related to AF_UNIX or AF_LOCAL address family : sockets for local IPC (Unix domain name sockets)
            //    = > bug
            case EIO:
            case ELOOP:         // only on IPC sockets (Unix domain name sockets) = > bug
            case ENAMETOOLONG:  // only on IPC sockets (Unix domain name sockets) = > bug
            case EACCES:        // only on IPC sockets (Unix domain name sockets) = > bug
            case ENOENT:
            case ENOTDIR:
            default:
                assert(0);  // should NEVER happen
                break;
        }
    }
}

MsgSocket::ErrorType 
ClientSocket::
_getRxErrorType(int err) 
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
                err_type =  MsgSocket::_getRxErrorType(err);
            break;
                
        default:     
            // default rtreatment.
            err_type =  MsgSocket::_getRxErrorType(err);
            break;
    }

    return err_type;
}

MsgSocket::ErrorType 
ClientSocket::
_getTxErrorType(int err) 
{
    MsgSocket::ErrorType err_type;
    
    switch (err)
    {
        default:     
            // default treatment.
            err_type =  MsgSocket::_getTxErrorType(err);
            break;
    }

    return err_type;
}



void
ClientSocket::
_setState(State    new_state) 
{
    switch(sock_state)
    {
        case MsgSocket::State::SOCK_CONNECTING:
            
            if (new_state == MsgSocket::State::SOCK_CONNECTED)
            {
            }
            break;
            
        default:
            break;
    }
    // call the parent's method
    MsgSocket::_setState(new_state); 
}


bool 
ClientSocket::
poll()
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
    
    std::cout << "*";
    
    if (stat < 0)
    {
        // An abnormal situation happened, and the cause is to read in errno.
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
                return false;
            default:
                assert(0);
        }
    }
    else if (stat == 0)
    {
        // nothing in revent. So nothing to do.
        return false;
    }
    else if (fds.revents & POLLHUP)
    {
        _setState(MsgSocket::State::SOCK_DISCONNECTED);
        throw socket_disconnection{std::string{"socket disconnection (POLLHUP)"} };
    }
    else if (fds.revents & POLLERR)
    {
        _determineErrorType();
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
                _setState(MsgSocket::State::SOCK_CONNECTED);
            }
            
            sendNextData();
        }
    }
    return incoming_msg;
    
}


