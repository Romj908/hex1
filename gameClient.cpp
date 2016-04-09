/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <cstdlib>
#include <iostream>

#include "hex1Protocol.h"

#include "gameServer.h"
#include "gameClient.h"




void client_loop()
{
        // initialize the client's main objects.
    ;   

}


void client_main(const char *ip_interface_name)
{
        // initialize the client's main objects.
    //...
    struct sockaddr_in serverAddrBuffer;
    
    display_network_addresses();
    
    // if neither the requested interface nor the local loopback inteface (localhost) work then exit.
    if (get_ipv4_address(&serverAddrBuffer, const_cast<char *>(ip_interface_name)) <0
        && get_ipv4_address(&serverAddrBuffer,  const_cast<char *>("lo")) < 0)
    {
        throw "no Network interface" ;
    }
    
    // 
    try
    {
        client_loop();
    }
    // handling al the exceptions, in particular the shutdown of the server itself
    catch(...)
    {
        // 
    }

}