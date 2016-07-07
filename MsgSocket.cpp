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

std::ostream& operator<< (std::ostream& o, const peer_disconnection& obj)
{
    std::string str1 {" errno: "};    
    o << std::endl << obj.err_msg << str1 << obj.sys_errno << std::flush;
    return o;
}



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
    this -> _rxReset();
    this -> _txReset();
}

void
MsgSocket::
_setState(State    new_state) 
{
    sock_state = new_state; 
}

void
MsgSocket::
_txReset()
{
    tx_state = TxState::TX_INIT;
    tx_length = 0;
    tx_msg_length = 0;
    tx_msg.reset();
    tx_l1_msg_id = ClientServerL1MessageId::NONE;
}

void
MsgSocket::
_rxReset()
{
    rx_state = RxState::RX_INIT;
    rx_lenght = 0;
    
}
void
MsgSocket::
discardDataBuffers()
{
    this -> _txReset();
    rx_fifo.clear();
    
    this -> _rxReset();
    tx_fifo.clear();
    
    std::cout << "\nSocket queues have been cleared.";
}

void
MsgSocket::
stop()
{
    std::cout << "\nMsgSocket::stop";
    this -> discardDataBuffers();
    sock_state = State::SOCK_NULL;
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
    if (nb_sent < 0)
    {
        nb_sent = -errno;
        if ( nb_sent == -EWOULDBLOCK)
        {
            tx_state = TxState::TX_BUSY;
            std::cout << "MsgSocket::_socketSend TxState <- TX_BUSY";
        }
        else
        {
            if (_getTxErrorType(errno) == ErrorType::DISCONNECTED)
            {
                throw peer_disconnection{"MsgSocket::_socketSend", errno};                
            }
        }
    }
    
    return nb_sent;
}

void 
MsgSocket::
_sendLoop()
{
    ClientServerRequestPtr req_ptr = tx_fifo.front();
    ssize_t nb_sent;
    int length = 0;
    
    while (tx_state == TxState::TX_READY && !tx_fifo.empty())
    {
        
        if (req_ptr->someDataToSend())
        {
            if (!tx_msg) // bool operator.
            {
                // retrieve the next L1 message 
                int length = 0;
                tx_msg = req_ptr->buildNextL2Msg(tx_l1_msg_id, length);
                std::cout << "MsgSocket::_sendLoop() next msg:" << static_cast<unsigned short>(tx_l1_msg_id) <<" length:"<< length<< std::endl;
                tx_msg_length = length+sizeof(tx_header);
                // knowing the length of the message we can build the L1 header
                tx_header.id.val   = htons(static_cast<ClientServerIdentity_t>(tx_l1_msg_id));
                tx_header.lenght = htonl(tx_msg_length);  // total lenght of the message 
                tx_header.from = this->from;  // one single byte.
            }
            if (tx_length == 0)
            {
                // send the L1 header
                nb_sent = _socketSend(reinterpret_cast<char *>(&tx_header), sizeof(tx_header));
                if (nb_sent < 0)
                {
                    break; // leave the while()
                }
                else
                    tx_length += nb_sent;
            }
            if (tx_length < tx_msg_length)
            {
                // send the rest of the message
                nb_sent = _socketSend(reinterpret_cast<char *>(tx_msg.get()), tx_msg_length - tx_length );
                if ( nb_sent == EWOULDBLOCK)
                {
                    tx_state = TxState::TX_BUSY;
                    break; // leave the while()
                }
                else
                {
                    tx_length += nb_sent;
                }
            }
            if (tx_length == tx_msg_length)
            {
                std::cout << "MsgSocket::_sendLoop() completed msg " << static_cast<unsigned short>(tx_l1_msg_id);
                // the current message has been fully sent. Prepare to request the next one.
                _txReset();      
            }
        }
        else
        {
            // request has been served. Prepare move to the next one. 
            assert(tx_length == 0);    // we must have fully sent the precedent message. 
            assert(tx_msg_length == 0);      
            tx_fifo.pop_front(); // remove the element from the pending queue
            tx_l1_msg_id = ClientServerL1MessageId::NONE;
        }
        
    }
    
}

int
MsgSocket::
sendNextData()
{
    // at this point the socket is forceably ready to send, so its state is indicated as write-ready
    tx_state = TxState::TX_READY;
    
    try
    {
        _sendLoop();    
    }
    catch (peer_disconnection tx_disc)
    {
        std::cout << ref(tx_disc);
        std::cout << " MsgSocket::sendNextData()";
        this -> discardDataBuffers();
        throw;
    }
    
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
        if (_getRxErrorType(errno)== MsgSocket::ErrorType::IGNORE)
        {
            nb_bytes = -errno;  // negative value returned
        }
        else
        {
            if (_getRxErrorType(errno) == ErrorType::DISCONNECTED)
            {
                throw peer_disconnection{"_socketReceive unexpected error", errno};                
            }
        }
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
    ClientServerLength_t lenght;
    int nb_recv = _socketReceive(reinterpret_cast<char *>(&lenght), sizeof(lenght), flags);
    if (nb_recv == sizeof(lenght) )
    {
        // Successfully reading. Definitely consume these bytes (wiothout MSG_PEEK).
        nb_recv = _socketReceive(reinterpret_cast<char *>(&lenght), sizeof(lenght));
        assert(nb_recv == sizeof(lenght));
        
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
            std::cout << "\nMsgSocket::_recvLoop() next msg length:"<< rx_msg->l1_header.lenght << std::endl;
        }
        // if not all messages have been received request the rest of the message.
        int nb = rx_msg->l1_header.lenght - rx_lenght;
        if (nb > 0)
        {
            char *raw = reinterpret_cast<char *>(rx_msg.get());
            raw += rx_lenght;
            nb = _socketReceive(raw, nb);  
            if (nb > 0)  // number of bytes effectively received.
            {
                rx_lenght += nb;
            }
            else 
            {
                // all bytes have been read, or an error occured.
                int err = errno;
                assert(errno == EWOULDBLOCK);
                rx_state == RxState::RX_IDLE;
                break;
            }
        }
        else
        {
            assert(nb == 0);    //  bug if <0
            // all the message has been read. Convert it now to host format and queue it.
            ClientServerL1MsgHeader* l1_hdr = &rx_msg->l1_header;
            l1_hdr->id.val =     ntohs(l1_hdr->id.val);
            std::cout << "\nMsgSocket::_recvLoop() msg msg id=" << l1_hdr->id.val << std::flush;
            //l1_hdr->from is one single byte.
            
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
    
    try
    {
        _recvLoop();    
    }
    catch (peer_disconnection rx_disc)
    {
        std::cout << rx_disc << " MsgSocket::recvNextData() disconnection";
        this -> discardDataBuffers();
        throw;
    }
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
_getRxErrorType(int err) 
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

MsgSocket::ErrorType 
MsgSocket::
_getTxErrorType(int err) 
{
    /*

        EAGAIN or EWOULDBLOCK

            The socket's file descriptor is marked O_NONBLOCK and the requested operation would block. 
        EBADF
            The socket argument is not a valid file descriptor. 
     * 
        ECONNRESET
            A connection was forcibly closed by a peer. 
     * 
        EDESTADDRREQ
            The socket is not connection-mode and no peer address is set. 
        EINTR
            A signal interrupted send() before any data was transmitted. 
        EMSGSIZE
            The message is too large to be sent all at once, as the socket requires. 
        ENOTCONN
            The socket is not connected or otherwise has not had the peer pre-specified. 
        ENOTSOCK
            The socket argument does not refer to a socket. 
        EOPNOTSUPP
            The socket argument is associated with a socket that does not support one or more of the values set in flags. 
        EPIPE
            The socket is shut down for writing, or the socket is connection-mode and is no longer connected. 
            In the latter case, and if the socket is of type SOCK_STREAM, the SIGPIPE signal is generated to the calling thread.

        The send() function may fail if:

        EACCES
            The calling process does not have the appropriate privileges. 
        EIO
            An I/O error occurred while reading from or writing to the file system. 
        ENETDOWN
            The local network interface used to reach the destination is down. 
        ENETUNREACH

            No route to the network is present. 
        ENOBUFS
            Insufficient resources were available in the system to perform the operation.
         */
    MsgSocket::ErrorType err_type;
    
    switch (err)
    {
        /* Even if EAGAIN or EWOULDBLOCK should not happen since we rely on (p)select(), well, let it be. */
        //case EAGAIN:
        case EWOULDBLOCK:
            err_type = MsgSocket::ErrorType::IGNORE;
            break;

        case EMSGSIZE:
            err_type = MsgSocket::ErrorType::IGNORE;
            break;
            
        /* Problem with the remote end. Better to release that connection and to wait for a reconnection from it. */
        case ECONNRESET:
        case ENOTCONN:
        case ETIMEDOUT:
        case EDESTADDRREQ:
        case EPIPE:
        case ENETDOWN:  // cable disconnected...
        case ENETUNREACH:// cable disconnected ? ...
            err_type =  MsgSocket::ErrorType::DISCONNECTED;
            break;

        /* Abnormal system errors*/
        case EIO:     // ??
        case ENOBUFS: // should never occur under normal conditions.
        case ENOMEM:  // should never occur under normal conditions.
            err_type =  MsgSocket::ErrorType::FATAL;
            break;
            

        /* Abnormal cases - Bugs */
        case EBADF:
        case EINTR:
        case EINVAL:
        case ENOTSOCK:
        case EOPNOTSUPP:
        default:     
            err_type =  MsgSocket::ErrorType::FATAL;
            break;
    }

    return err_type;
}

MsgSocket::ErrorType 
MsgSocket::
_determineErrorType()
{
    if (sock_state == State::SOCK_DISCONNECTED)
        return MsgSocket::ErrorType::DISCONNECTED;
    
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
    if (err == -1)
        return _getRxErrorType(errno);// usual errno.
    else
        return MsgSocket::ErrorType::IGNORE; // to be clarified
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

