/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <sys/select.h>
#include <signal.h>

#include "assert.h"
#include "ServerClientConnection.h"
#include "util/ipUtilities.h"
#include "MsgSocket.h"

ServerClientConnection::
ServerClientConnection(const struct sockaddr_in *clientAddr, 
                 int clientSocket )
: ipAddr(*clientAddr), state(State::EMPTY)
{
    servSock = new ServerSocket();
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
    servSock->readData();
}

int  
ServerClientConnection::
socketDataWrite()
{
    return servSock->writeData();
}


/*
    ServerSocketsRouter class
 
 */

ServerSocketsRouter::ServerSocketsRouter()
{
    nb_client_connections = 0;
    clientsocks_nfds = 0;
    FD_ZERO(clientsocks_fds);
    ::sigemptyset(&select_sigmask);
    
}

ServerClientCnxPtr ServerSocketsRouter::
CreateClientConnection(struct sockaddr_in &clientAddr, int &clientSock)
{
    // check the validity of the socket descriptor;
    if (FD_ISSET(clientSock, clientsocks_fds) )
    {
        throw "socket descr already in clientsocks_fds!";
    }
    // update the field descriptor bit field: 
    if (clientSock >= clientsocks_nfds)
    {
        clientsocks_nfds = clientSock+1;
    }
    FD_SET(clientSock, clientsocks_fds);
    
    // create the struct in charge of this connection
    ServerClientCnxPtr new_client(new ServerClientConnection(clientAddr, clientSock));
    
    // Registrate the new socket and check that this descriptor was not already used.
    std::pair<std::map::iterator, bool> inserted;
    inserted= socket_map.insert(std::make_pair(clientSock,new_client));
    
            /* Note that the created (temporary) pair object is "moved" to the
             internal ::map data thanks to the 2011+ following definition of map::insert():
                std::pair<iterator, bool>
                insert(const value_type& __x)
                { return _M_t._M_insert_unique(__x); }
             * 
             *    having in the _Rb_tree::_M_insert_unique() defined as :
                 #if __cplusplus >= 201103L
                     template<typename _Arg> _M_insert_unique(_Arg&& __v)
                #else
             *      ...
             * In other words, no reference is kept by the map object on some external object.
             */
    
    if (!inserted.second)
    {
        throw "socket_map.insert ERROR!";
    }
    
    nb_client_connections++;
    
    return new_client;

}

void
ServerSocketsRouter::
ReleaseClientConnection(ServerClientCnxPtr the_cnx)
{
    int clientSock = the_cnx->get_socket_descr();
    
    FD_CLR(clientSock, clientsocks_fds);
    FD_CLR(clientSock, select_writefds);
    
    if (clientSock+1 == clientsocks_nfds)
    {
        // update the number of significant bits by searching which is the socket having the highest bit.
        clientsocks_nfds = 0;    // by default, set it to 0.
        for (int s = clientSock-1; s > 0; s--)
        {
            if (FD_ISSET(s,clientsocks_fds))
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
    int i_max = this->select_errorfds/__NFDBITS; // number of significant words 
    for (int i= 0; i <= i_max; i++ )
    {
        to.fds_bits[i] = from.fds_bits[i];
    }    
}


int 
ServerSocketsRouter::
_fdsExtractList(std::list<hex1::socket_d> &the_list, ::fd_set &set )
{
    the_list.clear();
    int list_size = 0;
    int i_max = this->select_errorfds/__NFDBITS; // number of full words to scan
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
    
    for (SocketList::iterator iter : the_list)
    {
        hex1::socket_d      socket = *iter;
        
        std::cout << "\nerror signaled on socket " << socket;
        
        // route to the corresponding connection object. 
        ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket);
        if (client_cnx_ptr->socketError())
        {
            // Connection broken. clear the potential IO indicators on that socket. We won't serve them.
            if (FD_ISSET(socket, this->select_readfds))
            {
                FD_CLR(socket, this->select_readfds);
                nb++; // because this one counts as a treated descriptor
            }
            if (FD_ISSET(socket, this->select_writefds))
            {
                FD_CLR(socket, this->select_writefds);
                nb++; // because this one counts as a treated descriptor
            }
        }
        nb++;  // the current descriptor having the error has been handled.
    }
    return nb; // return the total number of descriptors which have been handled.
        
}

void
ServerSocketsRouter::
_serveSocketReceptions(SocketList &the_list )
{
    int nb = 0;
    for (SocketList::iterator iter : the_list)
    {
        hex1::socket_d      socket = *iter;
        // route to the corresponding connection object. 
        ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket);
        
        client_cnx_ptr->socketDataRead();
        nb++;  // the current one.
    }    
    return nb; // return the number of descriptors which have been handled.
}

void
ServerSocketsRouter::
_serveSocketEmissions(SocketList &the_list )
{
    int nb = 0;
    for (SocketList::iterator iter : the_list)
    {
        hex1::socket_d      socket = *iter;
        // route to the corresponding connection object. 
        ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket);
        
        int again = client_cnx_ptr->socketDataWrite();
        if (!again)
        {
            // this socket has currently no more data waiting to be sent. 
            FD_CLR(socket, this->select_writefds);
        }
        nb++;  // the current one.
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
        
        
        int nb_act = ::pselect(select_errorfds, 
                                &select_readfds, 
                                &select_writefds, // 
                                &select_errorfds, 
                                &select_timeout, 
                                &select_sigmask);
        
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
        
        assert(nb_act == 0);

}

