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

ClientConnection *ClientConnection::clientModeConnection = nullptr;


void 
ClientConnection::
handle_server_message(ClientServerL1MsgPtr p_msg)
{
    switch (p_msg->l1_header.id.l1_msg_id)
    {
        case ClientServerL1MessageId::CONNECTION_CNF:
        case ClientServerL1MessageId::CONNECTION_REJ:
            break;
            
        case ClientServerL1MessageId::LOGGING_CNF:
        case ClientServerL1MessageId::LOGGING_REJ:
            break;

        case ClientServerL1MessageId::REGISTRATE_CNF: 
        case ClientServerL1MessageId::REGISTRATE_REJ:
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
            
        case ClientServerL1MessageId::REGISTRATE_REQ:
        case ClientServerL1MessageId::LOGGING_REQ:
        case ClientServerL1MessageId::CONNECTION_REQ:
        default:
            break;

    }
}    

void 
ClientConnection::
setState(CnxState new_state)
{
    switch (new_state)
    {
        case CnxState::NO_CONNECTION: 
            break;
            
        case CnxState::CONNECTING: 
            break;
            
        case CnxState::CONNECTED: 
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

ClientConnection::
ClientConnection(const struct sockaddr_in &serverAddr, 
                std::string &ip_interface_name)

        : cnx_state{CnxState::NO_SOCKET},
        ip_interface_name{ip_interface_name}
{
        server_ip_addr = serverAddr;
        this -> configureSocket();
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
        
        setState(CnxState::CONNECTING);
        
        clientSocket->connect();
    }

}

void 
ClientConnection::
startConnection()
{
    // request the sending of a ConnectionReq to the server.
    ClientServerMsgBodyPtr l2_msg_ptr {new ClientServerMsgBody};
    ::memset(l2_msg_ptr->connection_req.client_version, 0, CLIENTSERVER_VERSION_STRING_LENGTH);
    
    ::strcpy(l2_msg_ptr->connection_req.client_version, mainVersionString().c_str());
        
    auto send_msg_req_p = new ClientServerMsgRequest(l2_msg_ptr, sizeof(ConnectionReq), 
                                                     ClientServerL1MessageId::CONNECTION_REQ);
    
    ClientServerMsgRequestUPtr req {send_msg_req_p};
    
    this -> sendMsgToServer(std::move(req));
    
}

void
ClientConnection::
peerDisconnected()
{
    setState(CnxState::NO_CONNECTION);
    
    // all outgoing/incoming data have already been flushed by the socket
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
        bool incoming_msg = clientSocket->poll();

        if (incoming_msg)
        {
            std::cout << "\nClientConnection::poll incoming_msg";
            ClientServerL1MsgPtr msg;
            do
            {
                msg = clientSocket->getNextReceivedMsg();

                handle_server_message(msg);

            } while (msg);
        }
    }
    catch (peer_disconnection disc)
    {
        peerDisconnected();
        throw; // propagate the exception to higher level
    }
    
}


void 
ClientConnection::
user_registration()
{
}

void 
ClientConnection::
sendMsgToServer(ClientServerMsgRequestUPtr&& msg_ptr)
{
    clientSocket->sendMsg(std::move(msg_ptr));
}




