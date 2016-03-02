/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "hexmap.h"

using namespace HexagonMapNSpace;

std::ostream& 
HexagonMapNSpace::operator<<(std::ostream& o, Hexagon& h)
{
    HexCoord coord;
    coord.l = h.getCoordl();
    o << " hex[" << coord.s.line << "][" << coord.s.column << "]";
    return o;
}

HexCoord::HexCoord(mIndex line,mIndex column)
{
    s.line = line;
    s.column = column;
}

HexCoord::HexCoord(Coordl l)
{
    this->l = l;
}

//using namespace Hexmap


const HexCoord 
Hexmap::dirToVect [2][NbDirections] = 
{
    // even lines
    [0]={
        [NEast]= {-1, +0},
        [SEast]= {+1, +0},
        [East]=  {+0, +1},
        [NWest]= {-1, -1},
        [West]=  {+0, -1},
        [SWest]= {+1, -1},
    },
    // odd lines
    [1]={
        [NEast]= {-1, +1},
        [SEast]= {+1, +1},
        [East]=  {+0, +1},
        [NWest]= {-1, +0},
        [West]=  {+0, -1},
        [SWest]= {+1, +0},
   }
};


// NEast, SEast, East , NWest, West, SWest

Hexmap::Hexmap(mIndex lines, mIndex columns)
{
    assert(lines>0 && columns>0);
    
    this->hex = new Hexagon*[lines];
    
    for (int line=0; line < lines; line++)
    {
        this->hex[line] = new Hexagon[columns];
        for (int col = 0; col < columns; col++)
        {
            HexCoord aux;
            aux.s.column = col;
            aux.s.line = line;
            Hexagon *h = &hex[line][col];
            h->coord.l = aux.l;
            h->terrain = NULL;
            h->units = NULL;
            h->wind.l = 0;
        }
    }
    
    nbLines = lines;
    nbColumns = columns;
}

Hexmap::~Hexmap()
{
    // release each column before to release the line array.
    for (int col = 0; col < nbColumns; col++)
    {
        delete [] hex[col];
    }
    delete [] hex;
}

bool   
Hexmap::inRange(Coordl l) const
{
    HexCoord aux(l);  
    return (aux.s.line >= 0 &&
            aux.s.column >= 0 &&
            aux.s.line < nbLines &&
            aux.s.column < nbColumns );
}

bool   
Hexmap::inRange(HexCoord& c) const
{
    return (c.s.line >= 0 &&
            c.s.column >= 0 &&
            c.s.line < nbLines &&
            c.s.column < nbColumns );
}

Hexagon& 
Hexmap::getHex(Coordl l)
{
    assert(inRange(l));
    HexCoord c(l); // asserts if the coordinates are out of range.
    return hex[c.s.line][c.s.column];
}

Hexagon& 
Hexmap::getHex(HexCoord& c)
{
    assert(inRange(c));
    return hex[c.s.line][c.s.column];
}

Coordl 
Hexmap::getVect(Hexagon& h1, Hexagon& h2) const
{
    HexCoord vect;
    vect.s.line = h2.coord.s.line   - h1.coord.s.line;
    vect.s.column = h2.coord.s.column - h1.coord.s.column;
    return vect.l;
}


Coordl 
Hexmap::getCoordL(Hexagon& h, const HexCoord& vect) const
{
    HexCoord aux;
    aux.s.line = h.coord.s.line + vect.s.line;
    aux.s.column = h.coord.s.column + vect.s.column;
    if (aux.s.line < 0 || aux.s.line >= nbLines 
       || aux.s.column < 0 || aux.s.column >= nbColumns)
        return aux.l;
    else
       return 0;
}

Hexagon* 
Hexmap::getHex(Hexagon& h, Direction dir) const
{
    HexCoord aux;
    
    const HexCoord& vect = dirToVect[h.coord.s.line & 1][dir];
    
    aux.l = getCoordL(h, vect);
    
    if (aux.l == 0)
        return NULL;
    else
        return &hex[aux.s.line][aux.s.column];
}

int 
Hexmap::hexDist(Hexagon& h1, Hexagon& h2)
{
    // compute the difference between their line numbers.
    int diff_lines = h2.coord.s.line - h1.coord.s.line;
    
    // compute the difference between the column numbers.
    int diff_cols = h2.coord.s.column - h1.coord.s.column;
    
    // first we know that the distance between both hexes cannot be inferior
    // to the difference between their line numbers.
    int abs_diff_l = abs(diff_lines);
    int thres;   // thres to know wether h2 is in either cone above of below h1. 
    
    if (diff_lines == 0)
    {
        // if both hexes are on the same line then the distance is trivial.
        return abs(diff_cols);
    }
    
    // for the rest of the computation we have to differentiate two cases 
    // depending whether h2 is having an even or odd line value.
    if ((h1.coord.s.line & 1) == 0)
    {// h1 is on an even line.
        if (diff_cols >= 0)
            thres = abs_diff_l/2; // rounded down.
        else
            thres = (abs_diff_l+1)/2; // rounded up.
    }
    else
    {// h1 is on an odd line
        if (diff_cols >= 0)
            thres = (abs_diff_l+1)/2; // rounded up.
        else
            thres = abs_diff_l/2; // rounded down.
    }
    
    if (abs(diff_cols) <= thres)
        return abs_diff_l;
    else
        return abs_diff_l + (abs(diff_cols)-thres);
}
