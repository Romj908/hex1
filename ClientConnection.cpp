/* 
 * File:   ClientConnection.cpp
 * Author: jlaclavere
 * 
 * Created on April 8, 2016, 12:32 PM
 */
#include <cstdlib>
#include <iostream>
#include <memory>
#include <fcntl.h>
#include <unistd.h>// getpid()

#include "hex1Protocol.h"

#include "gameClient.h"
#include "ClientConnection.h"

ClientConnection *ClientConnection::clientModeConnection = nullptr;


void ClientConnection::handle_server_message()
{
    
}
ClientConnection::ClientConnection(const struct sockaddr_in &serverAddr, 
        std::string &ip_interface_name)
        : server_addr(serverAddr), ip_interface_name(ip_interface_name)
{
}

void ClientConnection::socket_connection()
{
}


void ClientConnection::user_registration()
{
}

