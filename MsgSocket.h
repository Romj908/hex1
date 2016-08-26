/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   MsgSocket.h
 * Author: jlaclavere
 *
 * Created on May 9, 2016, 11:37 PM
 */

#ifndef MSGSOCKET_H
#define MSGSOCKET_H

#include <signal.h>
#include <deque>  
#include <mutex>  
#include <exception>  
#include <iostream>
#include "ClientServerRequest.h"

/*
 * 
 * MsgSocket
 * 
 */
class MsgSocket
{
protected:
    enum class State {
        SOCK_NULL = 0,
        SOCK_ERROR = 1,
        SOCK_DISCONNECTED = 2, // must be > SOCK_ERROR
        SOCK_CONNECTING,   // state not used when on server side.
        SOCK_CONNECTED,
    };
    
    State    sock_state;

    struct sockaddr_in          peer_ip_addr;
    int                         socket_descr;  // system handle to the socket descriptor.
    
    enum class TxState {
        TX_INIT,
        TX_READY,
        TX_BUSY
    };
    
    enum class RxState {
        RX_INIT,
        RX_IDLE,
        RX_DATA
    };
    
    TransferDirection           from;
    
    TxState                     tx_state;
    int                         tx_length;   // number of bytes of the current message that have been sent 
    int                         tx_msg_length;     // length, in number of bytes, of the current message, inluding the l1 header 
    ClientServerL1MessageId     tx_l1_msg_id;
    ClientServerMsgBodyPtr      tx_msg;      // pointer to the current L1 message's body under emission.
    ClientServerL1MsgHeader     tx_header;   // l1 message's L1 header
    
    RxState                     rx_state;    
    int                         rx_lenght;   // number of bytes of the current message that have been already received.
    ClientServerL1MsgPtr        rx_msg;      // pointer to the current L1 message under reception. 
    
    DataTransferId              ul_file_transfer_id;
    
    std::deque<ClientServerRequestPtr>    tx_fifo;
    std::mutex                            tx_fifo_mutex;
    
    std::deque<ClientServerL1MsgPtr>      rx_fifo;
    std::mutex                            rx_fifo_mutex;

    
    MsgSocket(MsgSocket&) = delete;  // prevent override of the copy constructor
    MsgSocket& operator=(MsgSocket&) = delete;  // prevent copy
    
    MsgSocket(MsgSocket&&) = delete;  // prevent move constructor (C++ 2011)
    MsgSocket& operator=(MsgSocket&&) = delete;  // prevent move (C++ 2011).
    
    virtual ~MsgSocket() = default;
    
protected:
    // the constructor
    MsgSocket(const struct sockaddr_in &serverAddr, 
              int sock_descr,
              TransferDirection sending_from);
    
public:
    virtual void 
    socketConfiguration();     // virtual because depending whether in client or server mode.
    
    int 
    getSocketDescr() {return socket_descr; }
    
    void 
    shutdown(); /* close the connection when the socket connection is active.*/
    
public:
    /* 
     * Write some pending data to the socket.
     * Returns whether new message(s) have been received 
     */
    virtual int
    sendNextData();
    
    /* 
     * Read any incoming pending data from the socket.
     * Returns whether new message(s) have been received 
     */
    virtual bool
    recvNextData();
    
    virtual DataTransferId 
    transferFile();

    virtual void 
    sendMsg(ClientServerMsgRequestUPtr&& msg_ptr);
    
    virtual ClientServerL1MsgPtr
    getNextReceivedMsg();
    
    virtual void
    discardDataBuffers();
    
    virtual void
    stop();  // stop any activity immediatly and free owned buffers.
    
    enum class ErrorType {
        IGNORE,
        DISCONNECTED,       // the remote has closed the connection. We may want to simply reconnect.
        CONNECTION_TROUBLES,// some (temporary or not) troubles. Don't kill the connection, but inform the user.
        FATAL   // fatal problem or bug. Impossible to continue.
    };

    virtual MsgSocket::ErrorType 
    _determineErrorType(); 
    
protected:
    virtual MsgSocket::ErrorType 
    _getRxErrorType(int sock_err);            // virtual since server and clients may behave diferently.
    
    virtual MsgSocket::ErrorType 
    _getTxErrorType(int sock_err);   // virtual since server and clients may behave diferently.
    
    void    
    _sendLoop();
    
    void 
    _recvLoop();
    
    ssize_t 
    _socketSend(char *buffer, int buffer_length, int flags=0);
    
    ClientServerL1MsgUPtr    
    _recvBeginNewMsg();
    
    ssize_t 
    _socketReceive(char *buffer, int buffer_length, int flags=0);
    
    virtual void
    _setState(State    new_state);
    
    virtual void
    _txReset();

    virtual void
    _rxReset();
    
private:
    /* obsolete code First implementation was  based on the use of the SIGIO signal because 
     * I replaced it by the use of ::select() and ::poll().  but could used elsewhere.  */
    void socket_configuration_SIGIO();      /*currently not used */
    void SignalSIGIOHandler(siginfo_t *pInfo); /*currently not used */
    static void SocketSignalsStaticHandler(int signalType,  siginfo_t *pInfo, void *); /*currently not used */
};


/*
 * socket_disconnection :
 * exception reported by a socket detecting that the the connection has been closed remotely or lost.
 * 
 */
class socket_disconnection : public std::exception
{
    std::string       err_msg;
    int               sys_errno;
    
public:
    virtual const char* what() const noexcept override
    {
        return err_msg.c_str();
    };
    
    socket_disconnection(std::string&& msg, int err = 0) 
    {
        
        err_msg = msg;
        err_msg += ", errno:";
        err_msg += err;
        sys_errno = err;
    };
    
    
    virtual ~socket_disconnection() = default;
	
    friend std::ostream& operator<< (std::ostream& o, const socket_disconnection& obj);
    
};

#endif /* MSGSOCKET_H */
