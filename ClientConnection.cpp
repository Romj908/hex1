/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ClientConnection.cpp
 * Author: jlaclavere
 * 
 * Created on April 8, 2016, 12:32 PM
 */
#include "assert.h"
#include "ClientConnection.h"
#include "util/ipUtilities.h"

ClientContext::
ClientContext(const struct sockaddr_in *clientAddr, 
                 int clientSocket)
: ipAddr(*clientAddr), socket(clientSocket), state(State::EMPTY)
{

}

ClientContext::~ClientContext() 
{
    std::cout << "\n~ClientContext() called";
}

int ClientContext::operator()() 
{
    std::cout<< "\nNew client running :" << boost::this_thread::get_id();
    
    for (int i=0; i<10; i++)
    {
        std::cout<< i << ", ";
        
    }
    ClientConnectionExitPoint thread_exit ( ClientCnxPtr(this),
                                            boost::this_thread::get_id() );
    boost::this_thread::at_thread_exit(thread_exit);
    std::cout<< "\nNew client ending :" << boost::this_thread::get_id();
    return 0;
}
d
int ClientConnectionExitPoint::operator ()()
{
    std::cout<< "\nNew thread completed :" << this->th_id;
    
    // send a signal to the server thread ? goal is to remove the client context from the map and to release it.
    return 0;
}

void ClientContext::deny_connection() 
{
    assert(state == State::EMPTY);
    state = State::REJECTING;
}

void ClientContext::wait_authentication() 
{
    assert(state == State::EMPTY);
    state = State::REGISTERING;
}

