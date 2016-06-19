/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <cstdlib>
#include <iostream>
#include <memory>
#include <signal.h>
#include <fcntl.h>

#include "gameClient.h"

#include "ClientConnection.h"


void client_init(const char *ip_interface_name)
{
    // initialize the client's main objects.
    struct sockaddr_in serverAddrBuffer;
    
    display_network_addresses();
    
    // if neither the requested interface nor the local loopback interface (localhost) work then exit.
    if (get_ipv4_address(&serverAddrBuffer, const_cast<char *>(ip_interface_name)) <0)
    {
        throw "Network interface unavailable";
    }    
    
    // create the thread dedicated to the handling of the connection in client mode.
    std::string     interfaceName(ip_interface_name);
    
    ClientConnection::createObject( serverAddrBuffer, interfaceName);   
    
    ClientConnection::object()->configureSocket();

}


void client_main(const char *ip_interface_name)
{
    
    //...
    // 
    try
    {
        // initialize the client's main objects.
        client_init(ip_interface_name);
        
    }
    // handling al the exceptions, in particular the shutdown of the server itself
    catch(std::exception e)
    {
        std::cout << "\nclient_main() exception " << e.what();
    }
    catch(...)
    {
        std::cout <<  "\nclient_main() exception!";
    }

}