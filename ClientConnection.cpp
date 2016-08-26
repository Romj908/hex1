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
#include <string.h>
#include <unistd.h>// getpid()

#include "hex1Protocol.h"

#include "gameClient.h"
#include "ClientConnection.h"
#include "main_version.h"

ClientConnection *
ClientConnection::
clientModeConnection = nullptr;

/*
 * The single constructor that's allowed.
 */
ClientConnection::
ClientConnection(const struct sockaddr_in &serverAddr, 
                std::string &ip_interface_name)

        : cnx_state{CnxState::NO_SOCKET},
        ext_event{},
        ip_interface_name{ip_interface_name},
        ciph_kea{},
        ciphering{ConnectionCnf::Cyphering::NO_CICPHERING},
        user_name{},
        user_passwd{},
        user_email{}
{
        server_ip_addr = serverAddr;
        this -> configureSocket();
}

void 
ClientConnection::
StartConnectionToServer()
{
    setState(CnxState::CONNECTING);

    clientSocket->connect();
    
    // we immediately send a connection req message through the socket, even if not yet up.
    // it will be queued and sent by the socket object once in CONNECTED state.
    sendConnectionReq();
        
}

void 
ClientConnection::
StopConnectionToServer(bool flush)
{
    setState(CnxState::DISCONNECTING);

    if (flush)
    {
        clientSocket->discardDataBuffers();
    }
    // we immediately send a connection req message through the socket, even if not yet up.
    // it will be queued and sent by the socket object once in CONNECTED state.
    sendDisconnectionReq();
        
}



void 
ClientConnection::
sendConnectionReq()
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
    
    const auto& current_version = mainVersionString();
    
    current_version.copy(l2_msg_ptr->connection_req.client_version,
                         current_version.length(), 0);
    l2_msg_ptr->connection_req.client_version[current_version.length()+1] = '\0';
    
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(ConnectionReq), 
                                                     ClientServerL1MessageId::CONNECTION_REQ);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToServer(std::move(req));
    
}


void 
ClientConnection::
handleConnectionCnf(ClientServerL1MsgPtr p_msg) 
{
    if (cnx_state == CnxState::CONNECTING)
    {
        /* normal path */
        setState(CnxState::CONNECTED_NO_CREDENTIALS);
        
        // save the ciphering key.
        ::cipheringKeyDataCopy(this->ciph_kea, p_msg->l1_body.connection_cnf.key);
        
        if (p_msg->l1_body.connection_cnf.ciphering != ConnectionCnf::Cyphering::NO_CICPHERING)
        {
            /* activate ciphering to_do */
        }

        //...
        
        // For the moment let's provide the user's credential now, since no UI is available.
        const std::string uname{"Alberto"};
        const std::string upasswd{"toto66"}; 
        const std::string email{"alberto.alberto@zazou.com"};  

        this -> RegistrationDataFromUserInterface(uname, upasswd, email );
 
        
    }
    else
    {
        // abnormal situation
        assert(0); // temp... to_do
    }
}

void 
ClientConnection::
handleConnectionRej(ClientServerL1MsgPtr p_msg)
{
    
    std::string txt {"\nConnectionRej received, cause "};
    
    switch (p_msg->l1_body.connection_rej.cause)
    {
        case ConnectionRej::NO_CAUSE:
            txt += "unknown";
            break;
            
        case ConnectionRej::BAD_CLIENT_VERSION:
        {
            txt += "BAD_CLIENT_VERSION: ";
            auto p_server_sw_version = reinterpret_cast<const char *>(p_msg->l1_body.connection_rej.server_sw_version);
            std::string server_version {p_server_sw_version, CLIENTSERVER_VERSION_STRING_LENGTH };
            txt += server_version;
            txt += std::string("\nplease update your software");            
        }
            break;
            
        case ConnectionRej::IP_ADDRESS_CONFLICT:
            txt += "IP_ADDRESS_CONFLICT";
            break;
            
        case ConnectionRej::UNEXPECTED_CONN_REQ:
            txt += "UNEXPECTED_CONN_REQ";
            break;
            
        default:
            assert(0);
    }
    std::cout << txt << std::endl;
    // inform upper layer (UI) to_do

}

void 
ClientConnection::
sendRegistrationReq()
{
       // request the sending of a ConnectionReq to the server.
        ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};

        // copy the credentials into the message to send. 
        // The length of the strings have already been checked in RegistrationDataFromUserInterface().
        user_name.copy(l2_msg_ptr->registration_req.user_name,
                                user_name.length(), 0);
        l2_msg_ptr->registration_req.user_name[user_name.length()+1] = '\0';
        
        user_passwd.copy(l2_msg_ptr->registration_req.password,
                                user_passwd.length(), 0);
        l2_msg_ptr->registration_req.password[user_passwd.length()+1]  = '\0';
        
        user_email.copy(l2_msg_ptr->registration_req.mail_address,
                                CLIENTSERVER_USER_MAIL_STRING_LENGTH, 0);
        l2_msg_ptr->registration_req.mail_address[user_email.length()+1]  = '\0';
        
        auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(RegistrationReq), 
                                                         ClientServerL1MessageId::REGISTRATION_REQ);

        ClientServerMsgRequestUPtr req {send_msg_req_p};

        this -> sendMsgToServer(std::move(req));
}



void 
ClientConnection::
handleRegistrationCnf(ClientServerL1MsgPtr p_msg)
{
    setState(CnxState::REGISTERED);
    // inform upper layer (UI) to_do
}
void 
ClientConnection::
handleRegistrationRej(ClientServerL1MsgPtr p_msg)
{
    setState(CnxState::CONNECTED_NO_CREDENTIALS);
    // inform upper layer (UI) to_do
    
}

void 
ClientConnection::
sendUserLoginReq()
{
       // request the sending of a ConnectionReq to the server.
        ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};

        this ->user_name.copy(l2_msg_ptr->user_logging_req.user_name,
                                CLIENTSERVER_VERSION_STRING_LENGTH, 0);
        
        this ->user_passwd.copy(l2_msg_ptr->user_logging_req.password,
                                CLIENTSERVER_USER_PASSWD_STRING_LENGTH, 0);
        
        auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(UserLoginReq), 
                                                         ClientServerL1MessageId::USER_LOGGING_REQ);

        ClientServerMsgRequestUPtr req {send_msg_req_p};

        this -> sendMsgToServer(std::move(req));
}


void 
ClientConnection::
handleUserLoginCnf(ClientServerL1MsgPtr p_msg)
{
    setState(CnxState::REGISTERED);
    // inform upper layer (UI) to_do
}
void 
ClientConnection::
handleUserLoginRej(ClientServerL1MsgPtr p_msg)
{
    setState(CnxState::AUTH_REJECTED);
    // inform upper layer (UI) to_do

}

void 
ClientConnection::
handleDisconnectionReq(ClientServerL1MsgPtr p_msg)
{
    // the server is shuttting down
    assert(p_msg->l1_body.disconnection_req.cause == DisconnectionReq::Cause::SERVER_SHUTDOWN);
    
    // inform the upper layers that the connection is going to an end (UI)
    // to_do
    
    // send the acknowledgement
    sendDisconnectionCnf();
    
    // wait for the socket closure (socket_disconnection exception)
    setState(CnxState::CLOSING);
    
}

void 
ClientConnection::
 sendDisconnectionCnf()
{
      // request the sending of a ConnectionReq to the server.
        ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};

        auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(DisconnectionCnf), 
                                                         ClientServerL1MessageId::DISCONNECTION_CNF);
        ClientServerMsgRequestUPtr req {send_msg_req_p};

        this -> sendMsgToServer(std::move(req));
    
}


void 
ClientConnection::
sendDisconnectionReq()
{
        ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};

        l2_msg_ptr->disconnection_req.cause = DisconnectionReq::Cause::USER_SHUTDOWN;
        
        auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(DisconnectionReq), 
                                                         ClientServerL1MessageId::DISCONNECTION_REQ);
        ClientServerMsgRequestUPtr req {send_msg_req_p};

        this -> sendMsgToServer(std::move(req));
}

void 
ClientConnection::
handleDisconnectionCnf(ClientServerL1MsgPtr p_msg)
{
    assert(cnx_state == CnxState::DISCONNECTING);
    clientSocket->stop();
    setState(CnxState::CLOSING);
}


void 
ClientConnection::
handle_server_message(ClientServerL1MsgPtr p_msg)
{
    switch (p_msg->l1_header.id.l1_msg_id)
    {
        case ClientServerL1MessageId::CONNECTION_CNF:
            handleConnectionCnf(p_msg);
            break;
            
        case ClientServerL1MessageId::CONNECTION_REJ:
            handleConnectionRej(p_msg);
            break;
            
        case ClientServerL1MessageId::USER_LOGGING_CNF:
            handleUserLoginCnf(p_msg);
            break;
            
        case ClientServerL1MessageId::USER_LOGGING_REJ:
            handleUserLoginRej(p_msg);
            break;

        case ClientServerL1MessageId::REGISTRATION_CNF: 
            handleRegistrationCnf(p_msg);
            break;
            
        case ClientServerL1MessageId::REGISTRATION_REJ:
            handleRegistrationRej(p_msg);
            break;

        case ClientServerL1MessageId::DISCONNECTION_REQ:
            handleDisconnectionReq(p_msg);
            break;
            
        case ClientServerL1MessageId::DISCONNECTION_CNF:
            handleDisconnectionCnf(p_msg);
            break;

        case ClientServerL1MessageId::POLL_REQ:
        case ClientServerL1MessageId::POLL_CNF:
            break;

        case ClientServerL1MessageId::DATA_TRANSFER_REQ:
        case ClientServerL1MessageId::TRANSFER_DATA_BLK:
            break;

        case ClientServerL1MessageId::FILE_TRANSFER_REQ:
            break;
            
        default:
            assert(0);
            break;

    }
}    

void 
ClientConnection::
setState(CnxState new_state)
{
    if (new_state != this->cnx_state)
    {
        std::cout << "\nClientConnection new_state " << static_cast<int>(new_state);
    }
    switch (new_state)
    {
        case CnxState::NO_CONNECTION: 
            break;
            
        case CnxState::CONNECTING: 
            assert(cnx_state == CnxState::NO_SOCKET);
            break;
            
        case CnxState::CONNECTED_NO_CREDENTIALS:
            assert(cnx_state == CnxState::CONNECTING || cnx_state == CnxState::REGISTERING);
            break;
            
        case CnxState::CONNECTED: 
            assert(cnx_state == CnxState::CONNECTED_NO_CREDENTIALS);
            break;
            
        case CnxState::REGISTERING: 
            assert(cnx_state == CnxState::CONNECTED);
            break;
            
        case CnxState::REGISTERED: 
            assert(cnx_state == CnxState::REGISTERING);
            break;
            
        case CnxState::CLOSING: 
            break;
            
        case CnxState::CNX_REJECTED: 
            assert(cnx_state == CnxState::CONNECTING);
           break;
           
        case CnxState::AUTH_REJECTED: 
             assert(cnx_state == CnxState::REGISTERING);
            break;
            
        case CnxState::CNX_ERROR: 
            break;
        
        case CnxState::NO_SOCKET:
        default: 
            break;
    }
    cnx_state = new_state;
}

void 
ClientConnection::
configureSocket()
{
    assert(!clientSocket);    // bool operator
    assert(cnx_state == CnxState::NO_SOCKET);
    
    int sock = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
        throw("CLient application's socket creation error");
    }
    else
    {
        clientSocket.reset(new ClientSocket(server_ip_addr, sock));
    }

}

void 
ClientConnection::
_setExtEvent(const CnxExtEvent ev)
{
    std::unique_lock<std::mutex>(this ->ext_mutext);
    this ->ext_event.insert(ev);
}

void 
ClientConnection::
_handleOneExtEvent(const CnxExtEvent ev)
{
    switch (ev)
    {
        case CnxExtEvent::NEW_CREDENTIALS:
            setState(CnxState::CONNECTED);
            sendRegistrationReq();
            setState(CnxState::REGISTERING);
            
            break;
            
        default:
            assert(0);
            break;
    }
}

void 
ClientConnection::
_handleExtEvents()
{
    std::unique_lock<std::mutex>(this ->ext_mutext);
    
    if (!ext_event.empty())
    {
        for (auto ev : this ->ext_event)
        {
            _handleOneExtEvent(ev);
        }

        this ->ext_event.clear();    
    }
}
 
/* CAUTION : this function could be called from the thread in charge of the UI, hence the lock. */
void 
ClientConnection::
RegistrationDataFromUserInterface(
        const std::string& uname, 
        const std::string& upasswd, 
        const std::string& email)
{
        std::lock_guard<std::mutex> registration_lock (this ->registration_mutext);
        
        this ->user_name = uname;
        this ->user_passwd = upasswd;
        this ->user_email = email;
        
        // reserve one char for the null character.
        if (this ->user_name.size() >= CLIENTSERVER_USER_NAME_LENGTH)
            this ->user_name.resize(CLIENTSERVER_USER_NAME_LENGTH-1);
        
        if (this ->user_passwd.size() >= CLIENTSERVER_USER_PASSWD_STRING_LENGTH)
            this ->user_passwd.resize(CLIENTSERVER_USER_PASSWD_STRING_LENGTH-1);
        
        if (this ->user_email.size() >= CLIENTSERVER_USER_MAIL_STRING_LENGTH)
            this ->user_email.resize(CLIENTSERVER_USER_MAIL_STRING_LENGTH-1);
        
        this ->_setExtEvent(CnxExtEvent::NEW_CREDENTIALS);
}



void
ClientConnection::
socketDisconnected(const socket_disconnection& disc)
{
    
    setState(CnxState::NO_CONNECTION);
    
    // all outgoing/incoming data have already been flushed by the socket
    
    // inform the upper level (for UI information, and eventually degraded mode/reconnection)
    // to_do
}

void 
ClientConnection::
poll(void)
{
    if (cnx_state <= CnxState::NO_CONNECTION)
    {
        return;
    }
    
    try
    {
        // check if some informations from other threads (UI) have to be served.
        _handleExtEvents();
        
        // serve the socket and any incoming message.
        bool incoming_msg = clientSocket->poll();

        if (incoming_msg)
        {
            std::cout << "\nClientConnection::poll incoming_msg";
            ClientServerL1MsgPtr msg;
            do
            {
                msg = clientSocket->getNextReceivedMsg();
                if (msg)
                {
                    handle_server_message(msg);                    
                }

            } while (msg);
        }
    }
    catch (socket_disconnection disc)
    {
        std::cout << disc.what();
        socketDisconnected(disc); // to_do
        //throw; // donn't propagate the exception to higher level
    }
    
}


void 
ClientConnection::
sendMsgToServer(ClientServerMsgRequestUPtr&& msg_ptr)
{
    clientSocket->sendMsg(std::move(msg_ptr));
}




