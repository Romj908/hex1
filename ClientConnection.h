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

#include <memory>

#include "util/ipUtilities.h"

class ClientContext
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
    ClientContext(const struct sockaddr_in *clientAddr, int socket);

    ClientContext(const ClientContext& orig) = delete;
    
    virtual ~ClientContext();
    
    // entry point of the dedicated thread.
    void operator()();
    
    in_addr_t get_in_addr_t() const {return ipAddr.sin_addr.s_addr; }
    
    void deny_connection();
    void wait_authentication();
    
private:

};

/**
 * Pointers are smart pointers
 */
typedef std::shared_ptr<ClientContext> ClientCnxPtr;


#endif /* CLIENTCONNECTION_H */

