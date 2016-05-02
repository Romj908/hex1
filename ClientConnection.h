/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientConnection.h
 * Author: jlaclavere
 *
 * Created on April 8, 2016, 12:32 PM
 */
#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <signal.h>
#include "util/ipUtilities.h"

class ClientConnection
{
    enum class State {
        SOCK_NULL,
        SOCK_CONNECTING,
        SOCK_CONNECTED,
        CNX_REJECTED,
        REGISTERING,
        AUTH_REJECTED,
        REGISTERED,
        SOCK_ERROR,
        CLOSING,
        SOCK_DISCONNECT
    };

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
    
    State                       state;
    TxState                     txState;
    RxState                     rxState;
    std::string                 ip_interface_name;
    struct sockaddr_in          server_addr;
    int                         client_socket;
    
    // one unique instance is allowed. Use a Singleton pattern.
    
    static ClientConnection *clientModeConnection; // the unique instance of the class.
    
    ClientConnection(ClientConnection&) = delete;  // prevent override of the copy constructor
    ClientConnection& operator=(ClientConnection&) = delete;  // prevent copy
    
    ClientConnection(ClientConnection&&) = delete;  // prevent move constructor (C++ 2011)
    ClientConnection& operator=(ClientConnection&&) = delete;  // prevent move (C++ 2011).
    
    
    ClientConnection() = delete;  // default constructor not used
    
    ClientConnection(const struct sockaddr_in &serverAddr, std::string &ip_interface_name);  // is private.
    
public:
    static void createObject(const struct sockaddr_in &serverAddr, std::string &ip_interface_name)
    {
        if (clientModeConnection == nullptr)
            clientModeConnection = new ClientConnection( serverAddr, ip_interface_name);
        
    }
    static ClientConnection *object() 
    {
        return clientModeConnection;
    };
    
    
public:
    void user_registration();
private:
    void handle_server_message();
    
public:
    
    void socket_connection();

    void SignalSIGIOHandler(siginfo_t *pInfo);

    static void SocketSignalsStaticHandler(int signalType, siginfo_t *pInfo, void *);
    
    
};

#endif
