/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <signal.h>

#include "assert.h"
#include "util/ipUtilities.h"
#include "ServerClientConnection.h"

ServerClientConnection::
ServerClientConnection(const struct sockaddr_in &clientAddr, int clientSocket )
: state(State::EMPTY)
{
    servSock = new ServerSocket(clientAddr, clientSocket);
}

ServerClientConnection::
~ServerClientConnection() 
{
    std::cout << "\n~ServerClientContext() called";
}

void ServerClientConnection::
deny_connection() 
{
    assert(state == State::EMPTY);
    state = State::REJECTING;
}

void 
ServerClientConnection::
wait_authentication() 
{
    assert(state == State::EMPTY);
    state = State::REGISTERING;
}

void 
ServerClientConnection::
remoteInitialedClose()
{
    // to_do
}

void 
ServerClientConnection::
serverInitiatedClose()
{
    // to_do
}

bool 
ServerClientConnection::
socketError()
{
    bool clear_io_indicators = true;
    
    switch(this->servSock->handleError())
    {
        case MsgSocket::ErrorType::IGNORE:
            clear_io_indicators = false;
            break;
            
        case MsgSocket::ErrorType::DISCONNECTED:
            /* start closure of this connection  */
            this->remoteInitialedClose();
            break;
            
        case MsgSocket::ErrorType::FATAL:
            // handled by exception. We shouldn't get here.
            break;
    }
    return clear_io_indicators;
}

void 
ServerClientConnection::
socketDataRead()
{
    // the socket object has to read any pending incoming data and queue the received messages internally.
    servSock->recvNextData();

    do
    {
        ClientServerL1MsgPtr p_msg = servSock->getNextReceivedMsg();
        
        if (p_msg) // bool operator
        {
            handle_client_message(p_msg);
        }
        else
            break; // no more incoming message to consume
        
    }while (1);
}

int  
ServerClientConnection::
socketDataWrite()
{
    return servSock->sendNextData();
}

void 
ServerClientConnection::
handle_client_message(ClientServerL1MsgPtr p_msg)
{
    //using ClientServerL1MessageId;
    
    switch (p_msg->l1_header.id.l1_msg_id)
    {
        case ClientServerL1MessageId::CONNECTION_REQ:
            break;
            
        case ClientServerL1MessageId::LOGGING_REQ:
            break;

        case ClientServerL1MessageId::REGISTRATE_REQ:
            break;

        case ClientServerL1MessageId::DISCONNECT_REQ:
        case ClientServerL1MessageId::DISCONNECT_CNF:
        case ClientServerL1MessageId::DISCONNECT_IND:
            break;

        case ClientServerL1MessageId::POLL_REQ:
        case ClientServerL1MessageId::POLL_CNF:
            break;

        case ClientServerL1MessageId::DATA_TRANSFER_REQ:
        case ClientServerL1MessageId::TRANSFER_DATA_BLK:
            break;

        case ClientServerL1MessageId::FILE_TRANSFER_REQ:
            break;
        
        /*ABNORMAL CASES*/
        case ClientServerL1MessageId::CONNECTION_CNF:
        case ClientServerL1MessageId::CONNECTION_REJ:
        case ClientServerL1MessageId::LOGGING_CNF:
        case ClientServerL1MessageId::LOGGING_REJ:
        case ClientServerL1MessageId::REGISTRATE_CNF: 
        case ClientServerL1MessageId::REGISTRATE_REJ:
        default:
            // these messages are unexpected. Probably a bug.
            assert(0);
            break;

    }
}    

/*
    ServerSocketsRouter class
 
 */
ServerSocketsRouter *ServerSocketsRouter::serverSockRouterObject;

ServerSocketsRouter::
ServerSocketsRouter()
{
    nb_client_connections = 0;
    clientsocks_nfds = 0;
    FD_ZERO(&clientsocks_fds);
    ::sigemptyset(&select_sigmask);
    
}

ServerClientCnxPtr 
ServerSocketsRouter::
CreateClientConnection(struct sockaddr_in &clientAddr, hex1::socket_d  clientSock)
{
    // check the validity of the socket descriptor;
    if (FD_ISSET(clientSock, &clientsocks_fds) )
    {
        throw "socket descr already in clientsocks_fds!";
    }
    // update the field descriptor bit field: 
    if (clientSock >= clientsocks_nfds)
    {
        clientsocks_nfds = clientSock+1;
    }
    FD_SET(clientSock, &clientsocks_fds);
    
    // create the object in charge of this connection
    ServerClientCnxPtr p_new_client(new ServerClientConnection(clientAddr, clientSock));
    
    // Registrate the new socket and check that this descriptor was not already used.
    // std::pair<std::map::iterator, bool> inserted;
    auto inserted= socket_map.insert(std::make_pair(clientSock,p_new_client));
    
            /* Note that the created (temporary) pair object is "moved" to the
             internal ::map data thanks to the 2011+ following definition of map::insert():
                std::pair<iterator, bool>
                insert(const value_type& __x)
                { return _M_t._M_insert_unique(__x); }
             * 
             *    having in the _Rb_tree::_M_insert_unique() defined as an universal reference:
                 #if __cplusplus >= 201103L
                     template<typename _Arg> _M_insert_unique(_Arg&& __v)
                #else
             *      ...
             * std::make_pair(clientSock,new_client) returning a rvalue, the universal reference decays to rvalue.
             * In other words, no reference is kept by the map object on some external object.
             */
    
    if (!inserted.second)
    {
        throw "socket_map.insert ERROR!";
    }
    
    nb_client_connections++;
    
    return p_new_client;

}

void
ServerSocketsRouter::
ReleaseClientConnection(ServerClientCnxPtr the_cnx)
{
    int clientSock = the_cnx->get_socket_descr();
    
    FD_CLR(clientSock, &clientsocks_fds);
    FD_CLR(clientSock, &select_writefds);
    
    if (clientSock+1 == clientsocks_nfds)
    {
        // update the number of significant bits by searching which is the socket having the highest bit.
        clientsocks_nfds = 0;    // by default, set it to 0.
        for (int s = clientSock-1; s > 0; s--)
        {
            if (FD_ISSET(s,&clientsocks_fds))
            {
                clientsocks_nfds = s+1;
                break;
            }
        }
    }
    // remove (erase) that connection object from the socked map collection.
    // Note that this erase() operation well calls the destructor of the both parts of the corresponding std::pair.
    // In other words the  ServerClientConnection object is also destroyed thanks to the shared_ptr (ServerClientCnxPtr).
    int nb = socket_map.erase(clientSock); 
    assert(nb == 1);
    nb_client_connections--;
    
}

void
ServerSocketsRouter::
_fdsCopy(::fd_set &from, ::fd_set &to )
{
    // FD_ZERO(&to);  "perhaps" cleaner... but not required.
    
    int i_max = this->clientsocks_nfds/__NFDBITS; // number of significant words 
    for (int i= 0; i <= i_max; i++ )
    {
        to.fds_bits[i] = from.fds_bits[i];
    }    
}

/* 
 * build the list of all the socket descriptors which are set in the indicated bitmap.
 */
int 
ServerSocketsRouter::
_fdsExtractList(std::list<hex1::socket_d> &the_list, ::fd_set &set )
{
    the_list.clear();
    int list_size = 0;
    int i_max = this->clientsocks_nfds/__NFDBITS; // number of full words to scan, rest is empty.
    int sock_id = 0;
    for (int i= 0; i <= i_max; i++ )
    {
        __fd_mask m = set.fds_bits[i];
        if (m != 0)
        {
            for (int j = 0; 
                    j < __NFDBITS; 
                    j++, m>>=1, sock_id++)
            {
                if (m & 1)
                {
                    the_list.push_back(sock_id);
                    list_size +=1;
                }
            }
        }
        else
            sock_id += __NFDBITS;
    }
    return list_size;
}

int
ServerSocketsRouter::
_handleSocketErrors(SocketList &the_list )
{
    int nb = 0;
    
    for (auto socket : the_list)
    {
        std::cout << "\nerror signaled on socket " << socket;
        
        // route to the corresponding connection object. 
        ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket);
        if (client_cnx_ptr->socketError())
        {
            // Connection broken. clear the potential IO indicators on that socket. We won't serve them.
            if (FD_ISSET(socket, &select_readfds))
            {
                FD_CLR(socket, &select_readfds);
                nb++; // because this one counts as a treated descriptor
            }
            if (FD_ISSET(socket, &select_writefds))
            {
                FD_CLR(socket, &select_writefds);
                nb++; // because this one counts as a treated descriptor
            }
        }
        nb++;  // the current descriptor having the error has been handled.
    }
    return nb; // return the total number of descriptors which have been handled.
        
}

int
ServerSocketsRouter::
_serveSocketReceptions(SocketList &the_list )
{
    int nb = 0;
    for (auto socket : the_list)
    {
        // route to the corresponding connection object. 
        ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket);
        
        // read incomig data and if complete messages have been received, handle them.
        client_cnx_ptr->socketDataRead();
        nb++;  // the current one.
    }    
    return nb; // return the number of descriptors which have been handled.
}

int
ServerSocketsRouter::
_serveSocketEmissions(SocketList &the_list )
{
    int nb = 0;
    for (auto socket : the_list)
    {
        // route to the corresponding connection object. 
            ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket); // throw std::out_of_range
            
            int again = client_cnx_ptr->socketDataWrite();
            if (!again)
            {
                // this socket has currently no more data waiting to be sent. 
                FD_CLR(socket, &select_writefds);
            }
        nb++;  // for the current socket's emission.
    }    
    return nb; // return the number of descriptors which have been handled.
}


void
ServerSocketsRouter::
ServeClientSockets(void)
{
    /* get which sockets have to be served by using the POSIX select()/pselect() function. */
    /*see http://linux.die.net/man/3/select*/
    
        SocketList error_list;
        SocketList write_list;
        SocketList read_list;
        
        // prepare the pselect()'s bitmasks parameters
        
        _fdsCopy(clientsocks_fds, select_readfds); // incoming data is possible on all existing client sockets.
        
        _fdsCopy(clientsocks_fds, select_errorfds); // errors are possible on all existing client sockets.
        
        // set the timeout to one second
        struct timespec current_time;
        int err = ::clock_gettime(CLOCK_MONOTONIC, &current_time);
        assert(err == 0);
        current_time.tv_sec +=1; // stay blocked one second max.
        
        
        int nb_act = ::pselect(clientsocks_nfds, 
                                &select_readfds, 
                                &select_writefds, // 
                                &select_errorfds, 
                                &select_timeout, 
                                nullptr /*&select_sigmask*/);
        
        try 
        {
            // if errors have been seen on some descriptors treat them first. 
            if (nb_act > 0)
            {
                _fdsExtractList(error_list, select_errorfds);
                nb_act -= _handleSocketErrors(error_list);
            }

            if (nb_act > 0)
            {
                // serve incoming data, if any
                _fdsExtractList(read_list, select_readfds);
                nb_act -= _serveSocketReceptions(read_list);
            }

            if (nb_act > 0)
            {
                // serve outgoing data, if any 
                _fdsExtractList(write_list, select_writefds);
                nb_act -= _serveSocketEmissions(write_list);
            }
        }
        catch (std::out_of_range)
        {
            // a socket descriptor not found!? There is a bug somewhere.
            throw;
        }
        
        assert(nb_act == 0);

}

