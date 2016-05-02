/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerClientConnection.h
 * Author: jlaclavere
 *
 * Created on May 2, 2016, 3:39 PM
 */

#ifndef SERVERCLIENTCONNECTION_H
#define SERVERCLIENTCONNECTION_H

#include <boost/thread/thread.hpp>
#include <memory>

#include "util/ipUtilities.h"

class ServerClientConnection
{
    enum class State {
        EMPTY,
        REJECTING,
        REGISTERING,
        CONNECTED,
        ERROR,
        CLOSING
        
    };

    State state;
    struct sockaddr_in  ipAddr;
    int socket;
    
public:
    //ClientConnection();
    ServerClientConnection(const struct sockaddr_in *clientAddr, int socket);

    ServerClientConnection(const ServerClientConnection& orig) = delete;
    
    virtual ~ServerClientConnection();
    
    // entry point of the dedicated thread.
    int operator()();
    
    in_addr_t get_in_addr_t() const {return ipAddr.sin_addr.s_addr; }
    
    void deny_connection();
    void wait_authentication();
    
private:

};

/**
 * Pointers are smart pointers
 */
typedef std::shared_ptr<ServerClientConnection> ServerClientCnxPtr;


/**
 * callable class used read is going to die.
 */

class ClientConnectionExitPoint
{
public:
    ServerClientCnxPtr context;
    boost::thread::id th_id;
    // exit point of the dedicated thread.
    int operator()();
    
    // exit point of all client thread - called through boost::this_thread::at_thread_exit() 
    ClientConnectionExitPoint(ServerClientCnxPtr ctx, boost::thread::id who) : context(ctx), th_id(who) {};
};


#endif /* SERVERCLIENTCONNECTION_H */


