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
#include <unistd.h>  // getpid()

#include "hex1Protocol.h"

#include "gameClient.h"
#include "ClientConnection.h"

ClientConnection *ClientConnection::clientModeConnection = nullptr;



void ClientConnection::SignalSIGIOHandler(siginfo_t *pInfo)
{
    /*
        The following values can be placed in pInfo->si_code for a SIGIO/SIGPOLL signal:
        POLL_IN Data input available.
        POLL_OUT Output buffers available (writing will not block)
        POLL_MSG Input system message available.
        POLL_ERR I/O error  at device level  (output only ? it's the case for poll() ).
        POLL_PRI High priority input available.
        POLL_HUP Hang up (output only) Device disconnected.
     */
    std::cout << "\nSignal SIGIO si_code " << pInfo->si_code << " ";
    switch (pInfo->si_code)
    {
        case POLL_IN:
            rxState = RxState::RX_DATA;
            state = State::SOCK_CONNECTED;
            break;
        case POLL_MSG:
            std::cout << " == POLL_MSG!";
            break;
        case POLL_PRI:
            std::cout << " == POLL_PRI!";
            break;
        case POLL_OUT:
            txState = TxState::TX_READY;
            state = State::SOCK_CONNECTED;
            break;
        case POLL_ERR:
            txState = TxState::TX_INIT;
            rxState = RxState::RX_INIT;
            state = State::SOCK_ERROR;
            break;
        case POLL_HUP:
            txState = TxState::TX_INIT;
            rxState = RxState::RX_INIT;
            state = State::SOCK_DISCONNECT;
            break;
        default:
            break;
    }
}

void ClientConnection::SocketSignalsStaticHandler(int signalType,  siginfo_t *pInfo, void *)
{
    switch(signalType)
    {
        case SIGIO:
            clientModeConnection->SignalSIGIOHandler(pInfo);
            break;
        case SIGPIPE:
            break;
        case EWOULDBLOCK:
            break;
        default:
            break;
    }
}

void ClientConnection::handle_server_message()
{
    
}
ClientConnection::ClientConnection(const struct sockaddr_in &serverAddr, 
        std::string &ip_interface_name)
        : server_addr(serverAddr), ip_interface_name(ip_interface_name)
{
    state = State::SOCK_NULL;
    txState = TxState::TX_INIT;
    rxState = RxState::RX_INIT;
}

void ClientConnection::socket_connection()
{
    int status;
    
    client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket < 0)
    {
        throw("Client's socket creation error");
    }

    /* configuring the socket to use non blocking SIGIO signals */
    struct sigaction    sigio_handler;
    sigio_handler.sa_flags = SA_SIGINFO ;
    sigio_handler.sa_sigaction = &ClientConnection::SocketSignalsStaticHandler;
    status = sigfillset(&sigio_handler.sa_mask);
    if (status < 0)
    {
        throw("client socket's sigfillset() error");
    }
    
    status = sigaction(SIGIO, &sigio_handler, nullptr);
    if (status < 0)
    {
        throw("client socket's sigaction() error");
    }
    
    /* signals from that socket have to be sent to this process => take ownership */
    status = fcntl(client_socket, F_SETOWN, getpid());
    if (status < 0)
    {
        throw("client socket's ownership error");
    }
    
    /* Active non blocking mode and signals from that socket */
    status = fcntl(client_socket, F_SETFL, O_NONBLOCK | FASYNC);
    if (status < 0)
    {
        throw("client socket's fcntl() error");
    }
    
    std::cout << "Client starting connection request...";
    
    state = State::SOCK_CONNECTING;
    
    status = connect( client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    
    if (status < 0)
    {
        if (status != EWOULDBLOCK)
            throw("client socket's connect() error");
    }
    else if (status > 0)
    {
        state = State::SOCK_CONNECTED;
    }
    // wait until competion of the establisment.
    while ( state == State::SOCK_CONNECTING) 
    {
        pause();  // wait for a signal
    };

    if (state != State::SOCK_CONNECTED)
    {
        throw("Connection failure!");
    }
}


void ClientConnection::user_registration()
{
}

