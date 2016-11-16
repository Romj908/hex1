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
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>

#include "gameClient.h"

#include "ClientConnection.h"
#include "hex1Protocol.h"
#include "util/signalUtilities.h"
#include "util/ipUtilities.h"

void client_ui_thread_f(void)
{
    int ui_loop_cnt=0;
    std::cout << "UI Thread executing\n";
    while ( ui_loop_cnt <20)
    {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << ":";
            ui_loop_cnt++;
    }
    std::cout << "UI Thread ending\n";
    std::cout.flush();
}

void client_cnx_init(const char *ip_interface_name)
{
    // initialize the client's main objects.
    struct sockaddr_in serverAddrBuffer;
    
    std::cout << "\nCLient trying to connect through interface " << ip_interface_name;
    
    display_network_addresses();
    
    // if neither the requested interface nor the local loopback interface (localhost) work then exit.
    if (get_ipv4_address(&serverAddrBuffer, const_cast<char *>(ip_interface_name)) <0)
    {
        throw "Network interface unavailable";
    }    
    serverAddrBuffer.sin_port = htons(HEX1_IP_PORT);
    
    // create the thread dedicated to the handling of the connection in client mode.
    std::string     interfaceName(ip_interface_name);
    
    ClientConnection::createObject( serverAddrBuffer, interfaceName);   
    
    ClientConnection::object()->StartConnectionToServer();
    
}

void client_bg_loop(bool& application_end)
{
    try
    {
        while (!application_end)
        {
            ClientConnection::object()->poll();

            waitUsec(5*100*1000);   // x*100 ms   

            std::cout << "." << std::flush;

        }
    }
    catch(std::exception e)
    {
        std::cout << "\nclient_bg_loop() exception " << e.what() << std::flush;
        throw;
    }

}



void client_background_main(const char *ip_interface_name, bool& application_end)
{
    try
    {
        // initialize the client's main objects.
        client_cnx_init(ip_interface_name);
        
        // main loop of the client application.
        client_bg_loop(application_end);
        
    }
    // handling al the exceptions, in particular the shutdown of the server itself
    catch(...)
    {
        std::cout <<  "\nclient_main() exception!";
        std::cerr.flush();
        std::cout.flush();
    }

}

void client_main(const char *ip_interface_name)
{
    bool application_end = false;    // temp
    
    // starts the UI thread
    std::thread client_ui_thread(client_ui_thread_f);
    //std::unique_ptr<std::thread> 
    
    // jump into the bqackground task's loop
    client_background_main(ip_interface_name, application_end);
    
    // synchronize both bg and UI threads at shutdown.
    client_ui_thread.join();
    
    std::cout << "client_main() shutdown\n";
    std::cout.flush();

    
}
