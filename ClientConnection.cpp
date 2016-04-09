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

#include "ClientConnection.h"
#include "util/ipUtilities.h"

ClientContext::
ClientContext(const struct sockaddr_in *clientAddr, 
                 const unsigned short clientPort)
: ipAddr(*clientAddr), socket(clientPort)
{
}

ClientContext::ClientConnection(const ClientContext& orig) {
}

ClientContext::~ClientContext() {
}

