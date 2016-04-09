/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   gameServer.h
 * Author: jlaclavere
 *
 * Created on April 7, 2016, 7:26 PM
 */

#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <memory>

#include "util/ipUtilities.h"

#include "ClientConnection.h"


class GameServer
{
    unsigned short      port;
    struct sockaddr_in  ipAddr;
    int server_socket;
    
    bool existing_connection() const;
    void wait_new_connection() const;
    void open_new_client_connection();
    void handle_client_message();
    void server_loop();
    
    GameServer(GameServer&) = delete;
    GameServer& operator=(GameServer&) = delete;
    GameServer(GameServer&&) = delete;
    
public:
    static const int SOCK_MAX_PENDING = 16; // Max number of pending incoming new connectiuons on the socket listen())
    
    GameServer() = default;
    GameServer(const struct sockaddr_in *serverAddr, 
               const unsigned short serverPort);
    
    void start();
    
    const struct sockaddr_in& 
    get_ipAddr() const { return ipAddr;};
    
    unsigned short 
    get_ipPort() const { return port;};
    
    
    
};



typedef std::shared_ptr<ClientContext> ClientCnxPtr;

extern GameServer *gameServer;

extern void server_main(const char *ip_interface_name = "lo");



#endif /* GAMESERVER_H */
