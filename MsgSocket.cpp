/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MsgSocket.cpp
 * Author: jlaclavere
 * 
 * Created on April 8, 2016, 12:32 PM
 */
#include <cstdlib>
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <unistd.h>// getpid()
#include "util/ipUtilities.h"

#include "hex1Protocol.h"

#include "MsgSocket.h"


MsgSocket::
MsgSocket(const struct sockaddr_in &serverAddr, 
        std::string &ip_interface_name,
        int socket_descr,
        TransferDirection sending_from ) : 

        peer_ip_addr(serverAddr), 
        ip_interface_name(ip_interface_name), 
        socket_descr(socket_descr),
        from(sending_from)
{
    sock_state = State::SOCK_NULL;
    txState = TxState::TX_INIT;
    rxState = RxState::RX_INIT;
}

void MsgSocket::
socket_configuration() 
{
    int status;
    
    
    /* Active socket's non blocking mode */
    status = ::fcntl(socket_descr, F_SETFL, O_NONBLOCK);
    if (status < 0)
    {
        throw("MsgSocket socket's fcntl() error");
    }
    
}

ssize_t MsgSocket::
_socketSend(char *buffer, int buffer_length)
{
    ssize_t nb_bytes = ::send(this->socket_descr, buffer, buffer_length, 0 /*default behavior*/);
    return nb_bytes;
}

void MsgSocket::
_sendNextMsg()
{
    while (txState == TxState::TX_READY && !tx_fifo.empty())
    {
        ClientServerRequestPtr req_ptr = tx_fifo.front();
        if (req_ptr->someDataToSend())
        {
            ClientServerL1MessageId l1_msg_id;
            int length;
            int nb_sent;
            ClientServerMsgPointer msg_ptr = req_ptr->buildNextMsg(l1_msg_id, length);
            
            // knowing the length of the message we can build and send the L1 header
            ClientServerL1MsgHeader header;
            header.id.u16   = htons(static_cast<uint16_t>(l1_msg_id));
            header.lenght = htonl(length+sizeof(header));  // total lenght of the message 
            header.from = this->from;
            
            nb_sent = _socketSend(reinterpret_cast<char *>(&header), sizeof(header));
            assert(nb_sent == sizeof(header) || nb_sent == EWOULDBLOCK);
            
            // send the rest of the message
            nb_sent = _socketSend(reinterpret_cast<char *>(msg_ptr.get()), length);
            assert(nb_sent == length || nb_sent == EWOULDBLOCK);
            
            if ( nb_sent == EWOULDBLOCK)
            {
                txState = TxState::TX_BUSY;
            }
            else
            {
                // remove the element from the pending queue
                tx_fifo.pop_front();
            }
        }
        
    }
    
}

virtual int
MsgSocket::
writeData()
{
    // at this point the socket is forceably ready to send, so its state is indicated as write-ready
    txState = TxState::TX_READY;
    
    _sendNextMsg();
    
    if (tx_fifo.empty())
        return 0;
    else
        return 1;
}

virtual int
MsgSocket::readData()
{
    return 0;
}

virtual MsgSocket::ErrorType 
MsgSocket::
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
    int err = ::recvmsg(socket, &msg_hdr, MSG_PEEK );
    // an error is expected on that socket, so -1 is expected.
    assert(err == -1);
    
    /*
        EAGAIN or EWOULDBLOCK
            The socket's file descriptor is marked O_NONBLOCK and no data is waiting to be received; 
     *      or MSG_OOB is set and no out-of-band data is available and either the socket's file descriptor is marked O_NONBLOCK 
     *      or the socket does not support blocking to await out-of-band data. 
        EBADF
            The socket argument is not a valid file descriptor. 
        ECONNRESET
            A connection was forcibly closed by a peer. 
        EINTR
            The recv() function was interrupted by a signal that was caught, before any data was available. 
        EINVAL
            The MSG_OOB flag is set and no out-of-band data is available. 
        ENOTCONN
            A receive is attempted on a connection-mode socket that is not connected. 
        ENOTSOCK
            The socket argument does not refer to a socket. 
        EOPNOTSUPP
            The specified flags are not supported for this socket type or protocol. 
        ETIMEDOUT
            The connection timed out during connection establishment, or due to a transmission timeout on active connection.

        The recv() function may fail if:

        EIO
            An I/O error occurred while reading from or writing to the file system. 
        ENOBUFS
            Insufficient resources were available in the system to perform the operation. 
        ENOMEM
            Insufficient memory was available to fulfill the request.
     */
    MsgSocket::ErrorType err_type;
    
    switch (errno)
    {
        /* Even if EAGAIN or EWOULDBLOCK should not happen since we rely on (p)select(), well, let it be. */
        case EAGAIN:
        case EWOULDBLOCK:
            err_type = MsgSocket::ErrorType::IGNORE;
            break;

        /* Problem with the remote end. Better to release that connection and to wait for a reconnection from it. */
        case ECONNRESET:
        case ENOTCONN:
        case ETIMEDOUT:
            err_type =  MsgSocket::ErrorType::DISCONNECTED;
            break;

        /* Abnormal system errors*/
        case EIO:     // ??
        case ENOBUFS: // should never occur under normal conditions.
        case ENOMEM:  // should never occur under normal conditions.
            throw "Unexpected socket error (system?) ";
            err_type =  MsgSocket::ErrorType::FATAL;
            break;
            

        /* Abnormal cases - Bugs */
        case EBADF:
        case EINTR:
        case EINVAL:
        case ENOTSOCK:
        case EOPNOTSUPP:
        default:     
            throw "Unexpected socket error (Bug?) ";
            err_type =  MsgSocket::ErrorType::FATAL;
            break;
    }

    return err_type;
}



ClientSocket::
connect()
{
    assert(sock_state == State::SOCK_NULL);
    
    std::cout << "Client starting connection request...";
    
    sock_state = State::SOCK_CONNECTING;
    
    int status = ::connect( socket_descr, (struct sockaddr *) &peer_ip_addr, sizeof(peer_ip_addr));
    
    if (status < 0)
    {
        if (status != EWOULDBLOCK)
            throw("client socket's connect() error");
    }
    else if (status > 0)
    {
        sock_state = State::SOCK_CONNECTED;
    }
    // wait until competion of the establisment.
    while ( sock_state == State::SOCK_CONNECTING) 
    {
        ::pause();  // wait for a signal
    };

    if (sock_state != State::SOCK_CONNECTED)
    {
        throw("Connection failure!");
    }

}


ClientSocket::
ClientSocket(sockaddr_in& serverAddr, std::string& ip_interface_name)
        : MsgSocket(serverAddr, ip_interface_name)
{
    this->from = TransferDirection::FROM_CLIENT;
    
    this->socket_descr = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (this->socket_descr < 0)
    {
        throw("Client's socket creation error");
    }
    else
        this->socket_configuration();
}



// OBSOLETE METHODS for signals - kept here for the momment as potentially/possibly reusable.

void MsgSocket::SignalSIGIOHandler(siginfo_t *pInfo) /*currently not used */
{
    /*
        The following values can be placed in pInfo->si_code for a SIGIO/SIGPOLL signal:
        POLL_IN Data input available.
        POLL_OUT Output buffers available (writing will not block)
        POLL_MSG Input system message available.
        POLL_ERR I/O error  at device level  (output only ? it's the case for poll() ).
        POLL_PRI High priority input available.
        POLL_HUP Hang up (output only) Device disconnected.
     */
    std::cout << "\nSignal SIGIO si_code " << pInfo->si_code << " ";
    switch (pInfo->si_code)
    {
        case POLL_IN:
            rxState = RxState::RX_DATA;
            sock_state = State::SOCK_CONNECTED;
            break;
        case POLL_MSG:
            std::cout << " == POLL_MSG!";
            break;
        case POLL_PRI:
            std::cout << " == POLL_PRI!";
            break;
        case POLL_OUT:
            txState = TxState::TX_READY;
            sock_state = State::SOCK_CONNECTED;
            break;
        case POLL_ERR:
            txState = TxState::TX_INIT;
            rxState = RxState::RX_INIT;
            sock_state = State::SOCK_ERROR;
            break;
        case POLL_HUP:
            txState = TxState::TX_INIT;
            rxState = RxState::RX_INIT;
            sock_state = State::SOCK_DISCONNECT;
            break;
        default:
            break;
    }
}
 /*currently not used */
void MsgSocket::SocketSignalsStaticHandler(int signalType,  siginfo_t *pInfo, void * /*currently not used */)
{
    switch(signalType)
    {
        case SIGIO:
            break;
        case SIGPIPE:
            break;
        case EWOULDBLOCK:
            break;
        default:
            break;
    }
}
void MsgSocket::socket_configuration_SIGIO() /*currently not used */
{
    int status;
    
    /* configuring the socket to use non blocking SIGIO signals */
    struct sigaction    sigio_handler;
    sigio_handler.sa_flags = SA_SIGINFO ; /* Invoke signal-catching function with
                                            three arguments instead of one.  */
    sigio_handler.sa_sigaction = &MsgSocket::SocketSignalsStaticHandler;
    status = ::sigfillset(&sigio_handler.sa_mask);
    if (status < 0)
    {
        throw("client socket's sigfillset() error");
    }
    
    status = ::sigaction(SIGIO, &sigio_handler, nullptr);
    if (status < 0)
    {
        throw("client socket's sigaction() error");
    }
    
    /* signals from that socket have to be sent to this process => take ownership */
    status = ::fcntl(socket_descr, F_SETOWN, getpid());
    if (status < 0)
    {
        throw("client socket's ownership error");
    }
    
    /* Active non blocking mode and signals from that socket */
    status = ::fcntl(socket_descr, F_SETFL, O_NONBLOCK | FASYNC);
    if (status < 0)
    {
        throw("client socket's fcntl() error");
    }
    
}

