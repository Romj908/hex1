/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <cstdlib>
#include <iostream>

#include "hexmap.h"
#include "assert.h"

using namespace std;
using namespace HexagonMapNSpace;

#define NB_test1_hexDist_results 20
static const int test1_Hexmap_hexDist_results[NB_test1_hexDist_results] = 
{
  6, //hex[7][6] to  hex[1][3] 6
  4, //hex[1][7] to  hex[2][4] 4
  3, //hex[1][5] to  hex[2][3] 3
  4, //hex[2][3] to  hex[3][6] 4
  2, //hex[4][2] to  hex[4][0] 2
  7, //hex[3][0] to  hex[7][5] 7
  1, //hex[6][2] to  hex[6][3] 1
  6, //hex[3][7] to  hex[1][2] 6
  6, //hex[6][2] to  hex[5][7] 6
  3, //hex[1][0] to  hex[3][2] 3
  2, //hex[5][5] to  hex[5][7] 2
  6, //hex[0][1] to  hex[6][4] 6
  5, //hex[3][2] to  hex[5][6] 5
  4, //hex[3][4] to  hex[4][1] 4
  2, //hex[6][2] to  hex[4][1] 2
  3, //hex[1][5] to  hex[4][7] 3
  6, //hex[0][1] to  hex[6][1] 6
  7, //hex[1][1] to  hex[4][7] 7
  6, //hex[6][1] to  hex[6][7] 6
  3, //hex[2][4] to  hex[3][6] 3
};
int test1_Hexmap_hexDist(void)
{
    mIndex kLines = 3;
    mIndex kColumn = 3;
    mIndex nbLines = 1<<kLines;
    mIndex nbColumns = 1<<kColumn;
    
    Hexmap theMap(nbLines,nbColumns);
    
    std::srand(1); // init of the random generator with the default value.
    
    for (int i=0; i<NB_test1_hexDist_results; i++)
    {
        HexCoord c;
        mIndex line = (mIndex) std::rand() & (nbLines-1);
        mIndex col =  (mIndex) std::rand() & (nbColumns-1);
        c.s.line = line;
        c.s.column = col;
        Hexagon& h1 = theMap.getHex(c.l);
        
        line = (int) std::rand() & (nbLines-1);
        col =  (int) std::rand() & (nbColumns-1);
        c.s.line = line;
        c.s.column = col;
        Hexagon& h2 = theMap.getHex(c.l);
        
        int dist = theMap.hexDist(h1, h2);
        
        cout <<"\n " << h1 << " to " << h2 << " : " << dist;
        
        assert(dist == test1_Hexmap_hexDist_results[i]);
    }
    
}
