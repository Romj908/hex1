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
#include <assert.h>
#include "util/ipUtilities.h"

#include "hex1Protocol.h"

#include "MsgSocket.h"


MsgSocket::
MsgSocket(const struct sockaddr_in &serverAddr, 
        int socket_descr,
        TransferDirection sending_from ) : 

        peer_ip_addr(serverAddr), 
        socket_descr(socket_descr),
        from(sending_from),
        rx_msg(nullptr)
{
    sock_state = State::SOCK_NULL;
    tx_state = TxState::TX_INIT;
    rx_state = RxState::RX_INIT;
    rx_lenght = 0;
    tx_length = 0;
    tx_msg.reset();
}

void 
MsgSocket::
socketConfiguration() 
{
    int status;
    
    /* Active socket's non blocking mode */
    status = ::fcntl(socket_descr, F_SETFL, O_NONBLOCK);
    if (status < 0)
    {
        throw("MsgSocket socket's fcntl() error");
    }
    
}

ssize_t 
MsgSocket::
_socketSend(char *buffer, int buffer_length, int flags)
{
    ssize_t nb_sent = ::send(this->socket_descr, buffer, buffer_length, flags);
    assert(nb_sent == sizeof(tx_header) || nb_sent == EWOULDBLOCK);
    return nb_sent;
}

void 
MsgSocket::
_sendLoop()
{
    ClientServerRequestPtr req_ptr = tx_fifo.front();
    ssize_t nb_sent;
    
    while (tx_state == TxState::TX_READY && !tx_fifo.empty())
    {
        if (req_ptr->someDataToSend())
        {
            if (!tx_msg) // bool operator.
            {
                // retrieve the next L1 message 
                ClientServerL1MessageId l1_msg_id;
                int length = 0;
                tx_msg = req_ptr->buildNextL2Msg(l1_msg_id, length);
                // knowing the length of the message we can build the L1 header
                tx_header.id.u16   = htons(static_cast<uint16_t>(l1_msg_id));
                tx_header.lenght = htonl(length+sizeof(tx_header));  // total lenght of the message 
                tx_header.from = this->from;
            }
            if (tx_length == 0)
            {
                // send the L1 header
                nb_sent = _socketSend(reinterpret_cast<char *>(&tx_header), sizeof(tx_header));
                
                if ( nb_sent == EWOULDBLOCK)
                {
                    tx_state = TxState::TX_BUSY;
                    break; // leave the while()
                }
                else
                    tx_length += nb_sent;
            }
            nb_sent = tx_length < tx_header.lenght;
            if (nb_sent > 0)
            {
                // send the rest of the message
                nb_sent = _socketSend(reinterpret_cast<char *>(tx_msg.get()), nb_sent);
                if ( nb_sent == EWOULDBLOCK)
                {
                    tx_state = TxState::TX_BUSY;
                    break; // leave the while()
                }
                else
                {
                    tx_length += nb_sent;
                    assert(tx_length <= tx_header.lenght);
                }
            }
            if (tx_length == tx_header.lenght)
            {
                // the current message has been fully sent. Prepare to request the next one.
                tx_msg.reset();      // release the pointer.
                tx_length = 0;
            }
        }
        else
        {
            // request has been served. Prepare move to the next one. 
            tx_fifo.pop_front(); // remove the element from the pending queue
            tx_msg.reset();      // release the pointer.
            tx_length = 0;
        }
        
    }
    
}

int
MsgSocket::
sendNextData()
{
    // at this point the socket is forceably ready to send, so its state is indicated as write-ready
    tx_state = TxState::TX_READY;
    
    _sendLoop();
    
    if (tx_fifo.empty())
        return 0;
    else
        return 1;
}

void 
MsgSocket::
sendMsg(ClientServerMsgRequestUPtr&& msg_ptr)
{
    ClientServerRequestPtr the_req{ std::move(msg_ptr) }; // a unique_ptr can be moved to a shared_ptr
    tx_fifo.push_back(the_req);
}

DataTransferId 
MsgSocket::
transferFile()
{
    // to_do
}

ssize_t 
MsgSocket::
_socketReceive(char *buffer, int buffer_length, int flags)
{
    ssize_t nb_bytes = ::recv(this->socket_descr, buffer, buffer_length, flags);
    
    if (nb_bytes <0)
    {
        nb_bytes = errno;
        assert(nb_bytes == EWOULDBLOCK);
    }
    return nb_bytes;
}

ClientServerL1MsgUPtr 
MsgSocket::
_recvBeginNewMsg(void)
{
    ClientServerL1MsgUPtr p_msg{nullptr};
    
    // read the length of the next message.
    int flags = MSG_PEEK;
    unsigned long lenght;
    int nb_recv = _socketReceive(reinterpret_cast<char *>(&lenght), sizeof(lenght), flags);
    if (nb_recv > 0 )
    {
        // Successfully reading. Definitely consume these bytes (wiothout MSG_PEEK).
        _socketReceive(reinterpret_cast<char *>(&lenght), sizeof(lenght));
        
        // allocate a new l1 msg buffer and set its lenght field.
        p_msg.reset(new ClientServerL1Msg);
        p_msg->l1_header.lenght = ::ntohl(lenght);
        
        // initialize the number of bytes read.
        this->rx_lenght = sizeof(lenght);
        
        return p_msg;
    }
    else
    {
        // not the expected amount of data. Simply wait 
        rx_state = RxState::RX_IDLE; 
    }
    return p_msg;
}

void 
MsgSocket::
_recvLoop()
{
    while (rx_state == RxState::RX_DATA)
    {
        // check if we are at the eve of a new l1 message. 
        //If yes, read it's length and allocate the buffer.
        if (rx_lenght == 0)
        {
            rx_msg.reset();
            rx_msg = _recvBeginNewMsg();
            if (!rx_msg)  // operator bool
            {
                rx_state == RxState::RX_IDLE;
                break;
            }
        }
        // if not all messages have been received request the rest of the message.
        int nb = rx_lenght <= rx_msg->l1_header.lenght;
        if (nb > 0)
        {
            char *raw = reinterpret_cast<char *>(rx_msg.get());
            nb = _socketReceive(raw, nb);  
            if (nb > 0)  // number of bytes effectively received.
            {
                rx_lenght += nb;
            }
            else 
            {
                // all bytes have been read, or an error occured.
                assert(nb == EWOULDBLOCK);
                rx_state == RxState::RX_IDLE;
                break;
            }
        }
        else
        {
            assert(nb == 0);    // some bug if <0
            
            // all the message has been read. Convert it now to host format and queue it.
            ClientServerL1MsgHeader* l1_hdr = &rx_msg->l1_header;
            l1_hdr->id.u16 =     ::ntohs(l1_hdr->id.u16);
            l1_hdr->from =   l1_hdr->from;
            
            // queue the message. Yield the ownership of the unique_ptr to the container.
            rx_fifo.push_back( std:: move(rx_msg));
            
            // prepare for the mext message
            rx_lenght = 0;
        }
    }
    
}

bool
MsgSocket::
recvNextData()
{
    // at this point the socket received forceably some data
    rx_state = RxState::RX_DATA;
    
    _recvLoop();

    return !rx_fifo.empty();
         
}

ClientServerL1MsgPtr
MsgSocket::
getNextReceivedMsg()
{
    ClientServerL1MsgPtr msg {nullptr };
    if (!rx_fifo.empty())
    {
        msg = rx_fifo.front();
        rx_fifo.pop_front();
    }
    return msg;
}
    
MsgSocket::ErrorType 
MsgSocket::
_getErrorType(int err) 
{
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
            Or a signal was received while blocked in the the accept() function (ansynchronous connection continuing) 
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
    
    switch (err)
    {
        /* Even if EAGAIN or EWOULDBLOCK should not happen since we rely on (p)select(), well, let it be. */
        //case EAGAIN:
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



// OBSOLETE METHODS for signals - kept here for the momment as potentially/possibly reusable.

void 
MsgSocket::
SignalSIGIOHandler(siginfo_t *pInfo) /*currently not used */
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
            rx_state = RxState::RX_DATA;
            sock_state = State::SOCK_CONNECTED;
            break;
        case POLL_MSG:
            std::cout << " == POLL_MSG!";
            break;
        case POLL_PRI:
            std::cout << " == POLL_PRI!";
            break;
        case POLL_OUT:
            tx_state = TxState::TX_READY;
            sock_state = State::SOCK_CONNECTED;
            break;
        case POLL_ERR:
            tx_state = TxState::TX_INIT;
            rx_state = RxState::RX_INIT;
            sock_state = State::SOCK_ERROR;
            break;
        case POLL_HUP:
            tx_state = TxState::TX_INIT;
            rx_state = RxState::RX_INIT;
            sock_state = State::SOCK_DISCONNECTED;
            break;
        default:
            break;
    }
}
 /*currently not used */
void 
MsgSocket::
SocketSignalsStaticHandler(int signalType,  siginfo_t *pInfo, void * /*currently not used */)
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
/*currently not used */
void 
MsgSocket::
socket_configuration_SIGIO() 
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

