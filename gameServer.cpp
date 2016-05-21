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

//#include <boost/thread/thread.hpp>
#include <fcntl.h>
#include <unistd.h>// getpid()

#include "hex1Protocol.h"

#include "gameServer.h"



GameServer *gameServer = nullptr;

GameServer::GameServer(const struct sockaddr_in *serverAddr, 
                        const unsigned short serverPort) 

            : port(serverPort), ipAddr(*serverAddr)

{
    ServerSocketsRouter::createObject();
}

void GameServer::SetupListenSocket()
{
    int status = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (status < 0)
    {
        throw("Server's socket creation error");
    }
    this->server_socket = status;

    status = ::bind( server_socket, (struct sockaddr *) &ipAddr, sizeof(ipAddr));
    if (status < 0)
    {
        throw("Server's bind() error");
    }
    
    // Allow TCP incoming connections on that socket
    status = ::listen( server_socket, SOCK_MAX_PENDING);
    
    if (status < 0)
    {
        throw("Server's listen() error");
    }
    /* Active non blocking mode on that socket */
    status = ::fcntl(server_socket, F_SETFL, O_NONBLOCK );
    if (status < 0)
    {
        throw("client socket's fcntl() error");
    }

}


bool GameServer::existing_connection() const
{
    // check if the indicated connection is already know.clientAddr
    return false;
}

// registrate a the new remote client 

void GameServer::handle_new_client_connection(ServerClientCnxPtr new_client)
{
    // is it a message from a new client or from a already registered one ?
//   ContextIterator found_item;    // map::find() returns an iterator on the found item.
//   found_item = clientsContexts.find( new_client->get_in_addr_t());
//
//   if (found_item != clientsContexts.end())
//   {
//       // message from a know client (same IP address). A connection already exist for it.
//       // reject that new request and let the existing run. Perhaps the player launched the client application twice.
//       // Had the distant experienced a crash, or some problems, just wait until a TCP alarm on the already exisiting 
//       // connection.
//       rejectedConnections.push_back(new_client); // queue all rejected connections in their list
//       new_client->deny_connection();
//   }
//   else
//   {
//       // normal case - no matching context => it's well a new session
//       // insert that context in a map container of the clients having an opened connections.
//       clientsContexts.emplace( std::make_pair(new_client->get_in_addr_t(), new_client) );
//
//       new_client->wait_authentication();
//   }
   // in both cases start a dedicated process to handle the socket and that session.

}


bool GameServer::IncomingConnectionRequest(struct sockaddr_in &clientAddr, int &clientSock) const
{
    // poll on the main socket for new clients or for messages from registered clients.
    int status;
    socklen_t addrLenght;
    addrLenght = sizeof(clientAddr);
    status = ::accept(server_socket, 
                        reinterpret_cast<struct sockaddr *>(&clientAddr), 
                        &addrLenght);
    if (clientSock < 0)
    {
        if (status == EAGAIN || status == EWOULDBLOCK)
            return false;  // no pending request in the listen() queue)
        else
            throw("Server's accept() error");
    }
    else
    {
        clientSock = status;
        return true;
    }
}

bool GameServer::ActiveClientSocket(int &clientSock)
{
    
    
    
    return false;
}


void GameServer::server_loop()
{
    while (1) // endless loop
    {
        try
        {
            // Wait a new TCP connection from any client and create a corresponding object.
            std::cout<< "\nserver_loop  1:";
            struct sockaddr_in clientAddr;
            int new_socket;
            if (IncomingConnectionRequest(clientAddr, new_socket))
            {
                ServerClientCnxPtr new_client;
                new_client= ServerSocketsRouter::object()->CreateClientConnection(clientAddr, new_socket );
            }
            pause(); // wait for any signal.
            
        }
        // handle socket exceptions.
        catch(boost::thread_resource_error e)
        {
            std::cout << "\nboost::thread_resource_error " << e.what();
        }
        // handle typical socket exceptions (broken pipe,...))
        catch(...)
        {
            std::cout<< "\nUnhandled Exception in GameServer::server_loop() :";
            
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
            std::cout<< "\nUnhandled Exception in GameServer::start() :";
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
            std::cout<< "\nserver_main :";
    
    gameServer->start();
}

