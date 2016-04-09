/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: jlaclavere
 *
 * Created on February 27, 2016, 6:25 PM
 */

#include <cstdlib>
#include <iostream>

#include "core/hexmap.h"
#include "util/circList.h"

using namespace std;


// Test functions:
extern int test1_circList(void);

extern int test2_circList(void);

#include "gameServer.h"
#include "gameClient.h"

/*
 * 
 */
int main(int argc, char** argv) 
{
    if (argc >= 2)
    {
        string cmd(argv[1]);
        if (cmd.compare("client")==0)
        {
            client_main(argv[2]);
        }
        else if (cmd.compare("server")==0)
        {
            if (argc == 3)
                server_main(argv[2]);
            else
                server_main();
                
        }
        else if (cmd.compare("tests")==0)
        {
            using namespace HexagonMapNSpace;
            test1_Hexmap_hexDist();
            test1_circList();
            test2_circList();
        }
    }
    else
    {
        std::cout << "\nERROR! usage : hex1 tests|client|master";
    }
    return 0;
}

