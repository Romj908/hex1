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
#include <list>  
#include "ClientServerRequest.h"

class ClientConnection
{
    enum class CnxState {
        NO_CONNECTION,
        CONNECTING,
        CONNECTED,
        REGISTERING,
        REGISTERED,
        CLOSING,
        CNX_REJECTED,
        AUTH_REJECTED,
        CNX_ERROR,
    };
    CnxState    cnx_state;

    /*
        one unique instance of this class is allowed. Use a Singleton pattern.
     */
    static ClientConnection *clientModeConnection; // the unique instance of the class.
    
    ClientConnection(ClientConnection&) = delete;  // prevent override of the copy constructor
    ClientConnection& operator=(ClientConnection&) = delete;  // prevent copy
    
    ClientConnection(ClientConnection&&) = delete;  // prevent move constructor (C++ 2011)
    ClientConnection& operator=(ClientConnection&&) = delete;  // prevent move (C++ 2011).
    
    ClientConnection() = delete;  // default constructor not used
    
    ClientConnection(const struct sockaddr_in &serverAddr, std::string &ip_interface_name);  // the constructor is private.
    
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
    void sendMsgToServer(ClientServerMsgPointer msg_ptr);
    ClientServerMsgPointer receiveMsgFromServer();
    
    void user_registration();
    
private:
    void handle_server_message();
    
public:
    
    
    
public:
};

#endif
