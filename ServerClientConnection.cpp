/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <signal.h>

#include "assert.h"
#include "util/ipUtilities.h"
#include "ServerClientConnection.h"
#include "main_version.h"

ServerClientConnection::
ServerClientConnection(const struct sockaddr_in &clientAddr, int clientSocket )
        : state(State::EMPTY), 
        last_rejected_cause {ConnectionRej::Cause::NO_CAUSE},
        user_pwd{},
        user_email{}        
{
    servSock = new ServerSocket(clientAddr, clientSocket);
}

ServerClientConnection::
~ServerClientConnection() 
{
    std::cout << "\n~ServerClientContext() called";
}


bool 
ServerClientConnection::
socketError()
{
    bool clear_io_indicators;
    
    switch(this->servSock->_determineErrorType())
    {
        case MsgSocket::ErrorType::IGNORE:
            clear_io_indicators = false; 
            break;
            
        case MsgSocket::ErrorType::DISCONNECTED:
            clear_io_indicators = true; 
            
            this->remoteInitialedClose();
            break;
            
        case MsgSocket::ErrorType::FATAL:
            clear_io_indicators = true; 
            
            this->fatalCnxError();
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
sendMsgToClient(ClientServerMsgRequestUPtr&& msg_ptr)
{
    std::cout << "\nsendMsgToClient #" << static_cast<ClientServerIdentity_t> (msg_ptr.get()->get_l1MsgId());
    
    // inform the router that some data have to be sent on that connection.
    ServerSocketsRouter::object()->OutgoingDataForClient(this->servSock->getSocketDescr());
    
    // queue the request to send the message
    servSock->sendMsg(std::move(msg_ptr));    
}


void 
ServerClientConnection::
sendConnectionCnf(ConnectionCnf::Cyphering ciph, const CipheringKeyData& ciph_key)
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
    
    l2_msg_ptr->connection_cnf.ciphering = ciph;
    if (ciph != ConnectionCnf::Cyphering::NO_CICPHERING)
    {
        ::cipheringKeyDataCopy(l2_msg_ptr->connection_cnf.key, ciph_key);
    }
    else
    {
        CipheringKeyData empty_key{};
        ::cipheringKeyDataCopy(l2_msg_ptr->connection_cnf.key, empty_key);
    }
    
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(ConnectionCnf), 
                                                     ClientServerL1MessageId::CONNECTION_CNF);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToClient(std::move(req));
    
}

void 
ServerClientConnection::
sendConnectionRej(ConnectionRej::Cause cause)
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
    
    l2_msg_ptr->connection_rej.cause = cause;
    
    auto p_version = reinterpret_cast<char *>(l2_msg_ptr->connection_rej.server_sw_version);
    const auto server_version = mainVersionString();
    server_version.copy(p_version, server_version.length());
    p_version[server_version.length()+1] = '\0';
    
    
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(ConnectionRej), 
                                                     ClientServerL1MessageId::CONNECTION_REJ);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToClient(std::move(req));
    
}


void 
ServerClientConnection::
handleConnectionReq(ClientServerL1MsgPtr p_msg) 
{
    std::cout << "\nCONNECTION_REQ, client version " << &p_msg->l1_body.connection_req.client_version[0];
    
    if (state == State::EMPTY)
    {
        // copy the version into a std::string for manipulations. It's null-terminated.
        std::string client_str{&p_msg->l1_body.connection_req.client_version[0]};
        
        if (client_str  == mainVersionString())
        {
            /* normal path */
            CipheringKeyData dummy_ciph_key{};    // to_do ciphering to be implemented
            sendConnectionCnf(ConnectionCnf::Cyphering::NO_CICPHERING, dummy_ciph_key );
            state = State::REGISTERING;
        }
        else
        {
            this ->last_rejected_cause = ConnectionRej::Cause::BAD_CLIENT_VERSION;
            sendConnectionRej(this ->last_rejected_cause);
            state = State::CONN_REJECTED;
        }
    }
    else if (state == State::CONN_REJECTED)
    {
        // abnormal situation
        sendConnectionRej(this ->last_rejected_cause);
        state = State::CONN_REJECTED;
    }
    else 
    {
        // abnormal situation
        this ->last_rejected_cause = ConnectionRej::Cause::UNEXPECTED_CONN_REQ;
        sendConnectionRej(this ->last_rejected_cause);
        state = State::CONN_REJECTED;
    }
}

bool passwd_ok(const std::string& user_name, const std::string& passwd)
{
    // look into files or DB
    //...  
    return true;// to_do
}

bool known_user(const std::string& user_name )
{
    // look into files or DB
    //...
    return false; // to_do
}

bool known_email(const std::string& user_email )
{
    // look into files or DB
    //...
    return false; // to_do
}

bool correct_email(const std::string& user_email )
{
    // look into files or DB
    //...
    return false; // to_do
}

void 
ServerClientConnection::
sendRegistrationRej(RegistrationRej::Cause cause)
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
    
    l2_msg_ptr->registration_rej.cause = cause;
    
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(RegistrationRej), 
                                                     ClientServerL1MessageId::REGISTRATION_REJ);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToClient(std::move(req));
    
}

void 
ServerClientConnection::
sendRegistrationCnf()
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
        
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(RegistrationCnf), 
                                                     ClientServerL1MessageId::REGISTRATION_CNF);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToClient(std::move(req));
    
}


void 
ServerClientConnection::
handleRegistrationReq(ClientServerL1MsgPtr p_msg) 
{
    if (state == State::REGISTERING)
    {
        /* normal path */
        // all the strings carried up in the message are null-terminated.
        auto p_name = reinterpret_cast<const char*>(p_msg->l1_body.registration_req.user_name);
        this ->user_name = std::string{p_name};
        
        auto p_email = reinterpret_cast<const char*>(p_msg->l1_body.registration_req.mail_address);
        this ->user_email = std::string{p_email};

        /* check if that user name is already registered */
        if (known_user(this ->user_name))
        {
            // name already existing!
            sendRegistrationRej(RegistrationRej::Cause::USER_ALREADY_EXISTING);
        }
        else if (correct_email(p_email))
        {
            // bad mail format!
            sendRegistrationRej(RegistrationRej::Cause::INCORRECT_EMAIL);
        }
        else
        {
            /* normal path */
            auto p_pwd = reinterpret_cast<const char*>(p_msg->l1_body.registration_req.password);
            this ->user_pwd = std::string{p_pwd};
            
            sendRegistrationCnf();
            
            state = State::REGISTERED;            
        }
        
    }
    else
    {
        // abnormal situation
        sendRegistrationRej(RegistrationRej::Cause::UNEXPECTED_REGISTRATION);
    }
}

void 
ServerClientConnection::
sendUserLoginRej(UserLoginRej::Cause cause)
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
    
    l2_msg_ptr->user_logging_rej.cause = cause;
    
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(UserLoginRej), 
                                                     ClientServerL1MessageId::USER_LOGGING_REJ);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToClient(std::move(req));
    
}

void 
ServerClientConnection::
sendUserLoginCnf()
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
        
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(UserLoginCnf), 
                                                     ClientServerL1MessageId::USER_LOGGING_CNF);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToClient(std::move(req));
    
}

void 
ServerClientConnection::
handleUserLoginReq(ClientServerL1MsgPtr p_msg) 
{
    if (state == State::REGISTERING)
    {
        /* normal path */
         // all the strings carried up in the message are null-terminated.
        auto p_name = reinterpret_cast<const char*>(p_msg->l1_body.registration_req.user_name);
        this ->user_name = std::string{p_name};
        auto p_pwd = reinterpret_cast<const char*>(p_msg->l1_body.registration_req.password);
        this ->user_pwd = std::string{p_pwd};
        
        /* check if that user is registered */
        if (!known_user(this ->user_name))
        {
            sendUserLoginRej(UserLoginRej::Cause::USER_UNKNOWN);
        }
        else if (!passwd_ok(this ->user_name, this ->user_pwd))
        {
            sendUserLoginRej(UserLoginRej::Cause::WRONG_PASSWORD);
        }
        else
        {
            // normal path
            sendRegistrationCnf();
            state = State::REGISTERED;                        
        }
    }
    else
    {
        // abnormal situation
        sendUserLoginRej(UserLoginRej::Cause::UNEXPECTED_LOGIN_PROC);
    }
}

void 
ServerClientConnection::
remoteInitialedClose()
{
    std::cout << "\nremoteInitialedClose Client connection #" << this->servSock;
    this->servSock->stop();
    state = State::DISCONNECTED;
}

void 
ServerClientConnection::
fatalCnxError()
{
    std::cout << "\fatalCnxError Client connection #" << this->servSock;
    this->servSock->stop();
    state = State::CNX_ERROR;
}


void 
ServerClientConnection::
serverInitiatedClose()
{
    // to_do
}

void 
ServerClientConnection::
handle_client_message(ClientServerL1MsgPtr p_msg)
{
    //using ClientServerL1MessageId;
    std::cout << "\nhandle_client_message " << p_msg->l1_header.id.val;
    
    switch (p_msg->l1_header.id.l1_msg_id)
    {
        case ClientServerL1MessageId::CONNECTION_REQ:
            handleConnectionReq(p_msg);
            break;
            
        case ClientServerL1MessageId::USER_LOGGING_REQ:
            handleUserLoginReq(p_msg);
            break;

        case ClientServerL1MessageId::REGISTRATION_REQ:
            handleRegistrationReq(p_msg);
            break;

        case ClientServerL1MessageId::DISCONNECTION_REQ:
        case ClientServerL1MessageId::DISCONNECTION_CNF:
        case ClientServerL1MessageId::DISCONNECTION_IND:
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
        case ClientServerL1MessageId::USER_LOGGING_CNF:
        case ClientServerL1MessageId::USER_LOGGING_REJ:
        case ClientServerL1MessageId::REGISTRATION_CNF: 
        case ClientServerL1MessageId::REGISTRATION_REJ:
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
: dead_list{}, socket_map{}
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
    std::cout << "\nCreating Client connection #" << clientSock;
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
                        insert(_Pair&& __x)
                        { return _M_t._M_insert_unique(std::forward<_Pair>(__x)); }
             * 
             *    having in the _Rb_tree::_M_insert_unique() defined as an universal reference:
                 #if __cplusplus >= 201103L
                     template<typename _Arg> _M_insert_unique(_Arg&& __v)
                #else
             *      ...
             * std::make_pair(clientSock,new_client) returning a rvalue, the universal reference decays to rvalue.
             * then the emptied temporary pair, on the stack, is deleted after to have been moved.
             * In other words, no reference is kept by the map object on some external pair object.
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
ReleaseClientConnection(hex1::socket_d clientSock)
{
    std::cout << "\nReleasing Client connection #" << clientSock;
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

/* method called from the ServerClientConnection class when a socket has to be polled for outgoing data*/
void 
ServerSocketsRouter::
OutgoingDataForClient(hex1::socket_d clientSock)
{
    ServerClientCnxPtr  client_cnx_ptr = socket_map.at(clientSock);

    assert(client_cnx_ptr);   // bool operator
    
    if (!FD_ISSET(clientSock, &select_writefds))
    {
        std::cout << "\nFlag outgoing data to Client connection #" << clientSock;
        FD_SET(clientSock, &select_writefds);
    }
    
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
        try
        {

            // read incomig data and if complete messages have been received, handle them.
            client_cnx_ptr->socketDataRead();
            nb++;  // the current one.
        }
        
        catch (peer_disconnection disc)
        {
            std::cout << disc << " on connection #" << socket;
            client_cnx_ptr -> remoteInitialedClose();
            dead_list.push_back(socket);
            
            nb++;  // the current one.
            /*throw*/; // DON'T propagate the exception to higher level!
        }
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
            ServerClientCnxPtr  client_cnx_ptr; 
            try
            {
                ServerClientCnxPtr  client_cnx_ptr = socket_map.at(socket); // throw std::out_of_range
                int again = client_cnx_ptr->socketDataWrite();
                if (!again)
                {
                    // this socket has currently no more data waiting to be sent. 
                    FD_CLR(socket, &select_writefds);
                }
                
            }
            catch (peer_disconnection disc)
            {
                std::cout << disc << " on connection #" << socket;
                client_cnx_ptr -> remoteInitialedClose();
                dead_list.push_back(socket);

                nb++;  // the current one.
                /*throw*/; // DON'T propagate the exception to higher level!
            }
            catch (std::out_of_range oor)
            {
                assert(0); // bug
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
        //_fdsCopy(clientsocks_fds, select_writefds); // see OutgoingDataForClient()
        _fdsCopy(clientsocks_fds, select_errorfds); // errors are possible on all existing client sockets.
        
        // set the timeout to one second
        struct timespec current_time;
        int err = ::clock_gettime(CLOCK_MONOTONIC, &current_time);
        assert(err == 0);
        current_time.tv_sec +=1; // stay blocked one second max.
        
        
        int nb_act = ::pselect(clientsocks_nfds, 
                                &select_readfds, 
                                &select_writefds,
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
                std::cout << "\nServeClientSockets emissions";
                // serve outgoing data, if any 
                _fdsExtractList(write_list, select_writefds);
                nb_act -= _serveSocketEmissions(write_list);
            }
        }
        catch (std::out_of_range)
        {
            // a socket descriptor not found!? There is a bug somewhere.
            std::cout << "\nServeClientSockets out_of_range exception!";
            throw;
        }
        
        assert(nb_act == 0);
        
        /* delete the connection objects that have experienced fatal errors or remote disconnections.*/
        if (! this->dead_list.empty())
        {
            for (auto sock_desc : this -> dead_list)
            {
                ReleaseClientConnection(sock_desc);
            }
            this -> dead_list.clear();
        }
}

