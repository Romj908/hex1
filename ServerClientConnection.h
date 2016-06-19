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

//#include <boost/thread/thread.hpp>
#include <memory>
#include <map>
#include <list>
#include <sys/select.h>
#include <iostream>
#include "ClientServerRequest.h"
#include "ServerSocket.h"

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
    
    // dedicated socket manager.
    ServerSocket *servSock;
    
    struct sockaddr_in  ipAddr;
    
public:
    //ServerSocketsRouter();
    ServerClientConnection(const struct sockaddr_in &clientAddr, int socket);

    ServerClientConnection(const ServerClientConnection& orig) = delete;
    
    virtual ~ServerClientConnection(); // remove that connection. 
        
    in_addr_t get_in_addr_t() const {return ipAddr.sin_addr.s_addr; }
    int       get_socket_descr() const {return servSock->getSocketDescr();}
    
    void deny_connection();
    void wait_authentication();
    
    void handle_client_message(ClientServerL1MsgPtr msg); // analyse the received message and route it accordingly
    
    bool socketError();  // An error occured on that connection. Handle it accordingly
    void socketDataRead();   // some incoming data have to be read
    int  socketDataWrite();  // some pending data can be written.
    
    void serverInitiatedClose();      // main server originated closure. Should rarely happen. Enter CLOSING state. 
    void remoteInitialedClose();      // Remote client has closed the socket.  and .
private:
    void _release();   // socket has been closed or was in error. Deregistering the connection. The object is going to be destroyed.

};

/**
 * Pointers are smart pointers
 */
typedef std::shared_ptr<ServerClientConnection> ServerClientCnxPtr;

/*
 * ServerSocketsRouter class.
 * 
 * 
 */
class ServerSocketsRouter
{
    typedef std::map<hex1::socket_d,ServerClientCnxPtr>   SocketMap;
    typedef SocketMap::iterator                           SocketMapIterator;
    
    typedef std::list<hex1::socket_d>                     SocketList;
    typedef SocketList::iterator                          SocketListIterator;
    
    /* map to retrieve the ServerClient bject in charge of a socket whose file descriptor is given. */
    SocketMap socket_map;
    
    int nb_client_connections;   // For clarity, but always equal to the number of elts in socket_map.
    
    // permanent bitmap of existing sockets, in the same format than for the call to ::pselect().
    // The server's listening socket doesn't appear in them.
    ::fd_set clientsocks_fds;    
    
    // highest value among the socket descriptors. First param of pselect().
    int      clientsocks_nfds;  
    
    // temporary bitmap of the sockets to be polled for incoming data. It's set to a copy of clientsocks_fds when calling ::pselect()
    ::fd_set select_readfds;
    
    // permanent bitmap of the sockets having data waiting to be sent. It's a subset of clientsocks_fds
    ::fd_set select_writefds;
    
    // temporary bitmap of sockets on error. 
    // no Out-Of-Band data currently being used (MSG_OOB) we have no URG signal. So only usual errors (connection loss,...).
    ::fd_set select_errorfds; 
    
    // permanent bitmask of masked signals. Should be empty because so far we don't use SIGIO (in particular).
    ::sigset_t select_sigmask; 
    
    struct ::timespec select_timeout;
    
    
    int _fdsExtractList(SocketList &the_list, ::fd_set &set );
    
    int _handleSocketErrors(SocketList &the_list );
    int _serveSocketReceptions(SocketList &the_list );
    int _serveSocketEmissions(SocketList &the_list );
    
    
    void _fdsCopy(::fd_set &from, ::fd_set &to );
    /*
        one unique instance of this class is allowed. Use a Singleton pattern.
     */
    static ServerSocketsRouter *serverSockRouterObject; // the unique instance of the class.
    
    ServerSocketsRouter(ServerSocketsRouter&) = delete;  // prevent override of the copy constructor
    ServerSocketsRouter& operator=(ServerSocketsRouter&) = delete;  // prevent copy
    
    ServerSocketsRouter(ServerSocketsRouter&&) = delete;  // prevent move constructor (C++ 2011)
    ServerSocketsRouter& operator=(ServerSocketsRouter&&) = delete;  // prevent move (C++ 2011).
    
    ServerSocketsRouter();  // the constructor is private.
    
    virtual ~ServerSocketsRouter() = default;  // the destructor is private.
        
public:
    static void 
    createObject()
    {
        if (serverSockRouterObject == nullptr)
            serverSockRouterObject = new ServerSocketsRouter();
        
    }
    static ServerSocketsRouter *
    object() 
    {
        return serverSockRouterObject;
    };

public:
    ServerClientCnxPtr 
    CreateClientConnection(struct sockaddr_in &clientAddr,  hex1::socket_d clientSock);
    
    void 
    ReleaseClientConnection(ServerClientCnxPtr the_cnx);
    
    void
    ServeClientSockets();
    
     
    
    
};


#endif /* SERVERCLIENTCONNECTION_H */


