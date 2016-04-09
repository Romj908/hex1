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
}

void ClientContext::operator()() 
{
    
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

