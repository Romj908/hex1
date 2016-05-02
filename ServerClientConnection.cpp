/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "assert.h"
#include "ServerClientConnection.h"
#include "util/ipUtilities.h"

ServerClientConnection::
ServerClientConnection(const struct sockaddr_in *clientAddr, 
                 int clientSocket)
: ipAddr(*clientAddr), socket(clientSocket), state(State::EMPTY)
{

}

ServerClientConnection::~ServerClientConnection() 
{
    std::cout << "\n~ServerClientContext() called";
}

int ServerClientConnection::operator()() 
{
    std::cout<< "\nNew client running :" << boost::this_thread::get_id();
    
    for (int i=0; i<10; i++)
    {
        std::cout<< i << ", ";
        
    }
    ClientConnectionExitPoint thread_exit ( ServerClientCnxPtr(this),
                                            boost::this_thread::get_id() );
    boost::this_thread::at_thread_exit(thread_exit);
    std::cout<< "\nNew client ending :" << boost::this_thread::get_id();
    return 0;
}

int ClientConnectionExitPoint::operator ()()
{
    std::cout<< "\nNew thread completed :" << this->th_id;
    
    // send a signal to the server thread ? goal is to remove the client context from the map and to release it.
    return 0;
}

void ServerClientConnection::deny_connection() 
{
    assert(state == State::EMPTY);
    state = State::REJECTING;
}

void ServerClientConnection::wait_authentication() 
{
    assert(state == State::EMPTY);
    state = State::REGISTERING;
}

