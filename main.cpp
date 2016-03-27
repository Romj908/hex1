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
using namespace HexagonMapNSpace;


// Test functions:
extern int test1_circList(void);

extern int test2_circList(void);


/*
 * 
 */
int main(int argc, char** argv) 
{
    
    //test1_Hexmap_hexDist();
    test1_circList();
    test2_circList();
    return 0;
}

