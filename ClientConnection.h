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
    struct sockaddr_in  ipAddr;
    
    int socket;
    
public:
    //ClientConnection();
    ClientContext(const struct sockaddr_in *clientAddr, 
                     const unsigned short clientPort);

    ClientContext(const ClientContext& orig) = delete;
    
    virtual ~ClientContext();
    
private:

};

/**
 * Pointers are smart pointers
 */
typedef std::shared_ptr<ClientContext> ClientCnxPtr;


#endif /* CLIENTCONNECTION_H */

