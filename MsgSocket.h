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
#include <list>  
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
        SOCK_NULL,
        SOCK_CONNECTING,   // state not used when on server side.
        SOCK_CONNECTED,
        SOCK_ERROR,
        SOCK_DISCONNECT
    };
    
    State    sock_state;

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
    TxState                     txState;
    RxState                     rxState;
    std::string                 ip_interface_name;
    struct sockaddr_in          peer_ip_addr;
    int                         socket_descr;  // system handle to the socket descriptor.
    DataTransferId              ul_file_transfer_id;
    
    std::list<ClientServerRequestPtr>       tx_fifo;
    std::list<ClientServerL1MsgPointer>     rx_fifo;
        
    MsgSocket(MsgSocket&) = delete;  // prevent override of the copy constructor
    MsgSocket& operator=(MsgSocket&) = delete;  // prevent copy
    
    MsgSocket(MsgSocket&&) = delete;  // prevent move constructor (C++ 2011)
    MsgSocket& operator=(MsgSocket&&) = delete;  // prevent move (C++ 2011).
    
    ~MsgSocket() = delete;  // default constructor not used
    
protected:
    // regular constructor
    MsgSocket(const struct sockaddr_in &serverAddr, 
              std::string &ip_interface_name,
              int sock_descr,
              TransferDirection sending_from);
    
public:
    virtual void 
    socket_configuration();     // virtual because depending whether in client or server mode.
    
    
    void 
    shutdown(); /* close the connection when the socket connection is active.*/
    
public:
    virtual int
    writeData();
    
    virtual int
    readData();
    
    enum class ErrorType {
        IGNORE,
        DISCONNECTED,
        FATAL
    };

    virtual MsgSocket::ErrorType 
    handleError(); 
    
    virtual void 
    DataTransferId transferFile();
    
private:
    void    _sendNextMsg();
    ssize_t _socketSend(char *buffer, int buffer_length);
    
private:
    /* obsolete code based on the use of the SIGIO signal because replaced by the use of select() and poll() */
    void socket_configuration_SIGIO();      /*currently not used */
    void SignalSIGIOHandler(siginfo_t *pInfo); /*currently not used */
    static void SocketSignalsStaticHandler(int signalType,  siginfo_t *pInfo, void *); /*currently not used */
};


/*
 * 
 * ClientSocket : socket handling of a client's socket, on client side.
 * 
 */
class ClientSocket : public MsgSocket
{
public:
     
    ClientSocket(const struct sockaddr_in &serverAddr, std::string &ip_interface_name, int sock_descr);
    
    // methods specific to the client mode.
    void connect();
    
};


/*
 * 
 * ServerSocket : socket handling of a client's socket, on server side.
 * (it's not the server's socket listening for new connections!)
 * 
 * 
 */
class ServerSocket : public MsgSocket
{
public:
    ServerSocket(const struct sockaddr_in &serverAddr, std::string &ip_interface_name);
    
    virtual void socket_configuration() override;

    // methods specific to the server mode.

};
#endif /* MSGSOCKET_H */

