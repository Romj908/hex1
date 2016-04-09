/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdlib>
#include <iostream>
#include <map>
#include <cstring> // memset()
#include <iostream>

#include <boost/thread/thread.hpp>

#include "hex1Protocol.h"

#include "gameServer.h"



GameServer *gameServer = nullptr;

GameServer::GameServer(const struct sockaddr_in *serverAddr, 
                        const unsigned short serverPort) 

            : port(serverPort), ipAddr(*serverAddr)

{
    int status = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (status < 0)
    {
        throw("Server's socket creation error");
    }
    this->server_socket = status;

    status = bind( server_socket, (struct sockaddr *) &ipAddr, sizeof(ipAddr));
    if (status < 0)
    {
        throw("Server's bind() error");
    }
    
    // Allow TCP incoming connections on that socket
    status = listen( server_socket, SOCK_MAX_PENDING);
    
    if (status < 0)
    {
        throw("Server's listen() error");
    }
}


void GameServer::handle_client_message()
{
   // analyse the received message and route it accordingly
   ;

}


bool GameServer::existing_connection() const
{
    // check if the indicated connection is already know.
    return false;
}

void GameServer::open_new_client_connection()
{
    // registrate a the new remote client 
    
    // create a new threat dedicated to that client. 
    // It will be waiting for input from the main socket through a wait queue 
    
}


ClientCnxPtr GameServer::wait_new_connection() const
{
    // poll on the main socket for new clients or for messages from registered clients.
    int clientSock;
    struct sockaddr_in clientAddr;
    socklen_t addrLenght;
    
    addrLenght = sizeof(clientAddr);
    clientSock = accept(server_socket, 
            reinterpret_cast<struct sockaddr *>(&clientAddr), 
            &addrLenght);
    
    if (clientSock < 0)
    {
        throw("Server's accept() error");
    }
    
    ClientCnxPtr new_client(new ClientContext(&clientAddr, clientSock));
    return new_client;
}



void GameServer::server_loop()
{
    while (1) // endless loop
    {
        try
        {
            // Wait a new TCP connection from any client and create a corresponding object.
            ClientCnxPtr new_client = wait_new_connection();
            
            // is it a message from a new client or from a already registerd one ?
            ContextIterator found_item;    // map::find() returns an iterator on the found item.
            found_item = clientsContexts.find( new_client->get_in_addr_t());
            
            if (found_item != clientsContexts.end())
            {
                // message from a know client (same IP address). A connection already exist for it.
                // reject that new request and let the existing run. Perhaps the player launched the client application twice.
                // Had the distant experienced a crash, or some problems, just wait until a TCP alarm on the already exisiting 
                // connection.
                new_client->deny_connection();
            }
            else
            {
                // normal case - no matching context => it's well a new session
                // insert that context in the list of clients having an opened connections.
                clientsContexts.emplace( std::make_pair(new_client->get_in_addr_t(), new_client) );
                
                new_client->wait_authentication();
            }
            // in both cases start a dedicated thread
            new_client
        }
        // handle typical socket exceptions (broken pipe,...))
        catch(boost::thread_resource_error e)
        {
            std::cout << "\nboost::thread_resource_error " << e.what();
        }
        catch(...)
        {
            
        }
    }
}

void
GameServer::start()
{
        // 
    try
    {
        server_loop();
    }
    // handling al the exceptions, in particular the shutdown of the server itself
    catch(...)
    {
        // 
    }

}
    

void server_main(const char *ip_interface_name)
{
    // initialize the server's main objects.
    struct sockaddr_in serverAddrBuffer;
    
    memset(&serverAddrBuffer, 0, sizeof(serverAddrBuffer));
    serverAddrBuffer.sin_family = AF_INET;
    serverAddrBuffer.sin_addr.s_addr = htonl(INADDR_ANY);   // don't care to retrieve the local server's address.
    serverAddrBuffer.sin_port = htons(HEX1_IP_PORT);
    ;   
    gameServer = new GameServer(&serverAddrBuffer, HEX1_IP_PORT);
    
    gameServer->start();
}

