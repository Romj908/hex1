/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   hexmap.h
 * Author: jlaclavere
 *
 * Created on February 27, 2016, 7:24 PM
 */

#ifndef HEXMAP_H
#define HEXMAP_H

#include <cstdlib>

#include <vector>
#include <ostream>
#include <assert.h>


namespace HexagonMapNSpace
{
    
    
typedef signed short mIndex;  // used as index in the map array and in translations.
typedef unsigned Coordl;

typedef struct {
        mIndex line, column;
    } HexCoords;
    
    
union HexCoord
{
public:
    HexCoords s;
    Coordl l;
    
    HexCoord() {l = 0;};
    HexCoord(mIndex line,mIndex column);
    HexCoord(Coordl l);
};

class Hexmap;

class Hexagon
{
protected:
    friend std::ostream& operator<<(std::ostream& o, Hexagon& h);
    friend Hexmap;
    
    HexCoord coord;
    void *terrain;
    void *units;
    HexCoord wind;
    
    Hexagon() {};
    
public:
    Coordl getCoordl() const { return coord.l; }

};

std::ostream& operator<<(std::ostream& o, Hexagon& h);


class Hexmap
{
public:
    enum Direction 
    {
        NEast, SEast, East , NWest, West, SWest, NbDirections
    };
protected:
    static const HexCoord dirToVect[2][NbDirections]; // [2] both even/odd lines.
    
protected:
    Hexagon **hex;
    mIndex nbLines;
    mIndex nbColumns;
    mIndex getNbLines() const {return nbLines; };
    mIndex getNbColumns() const {return nbColumns; };
    
    // test whether the indicated coordinates are well inside the map's limits.
    bool   inRange(Coordl l) const;
    bool   inRange(HexCoord& c) const;
    
public:
    Hexmap(mIndex nbLines, mIndex NbColumns);
    ~Hexmap();
    
    // get the reference of the hexagon at the given coordinates
    Hexagon& getHex(Coordl l); // the coordinates have to be valid else an assert fires.
    Hexagon& getHex(HexCoord& c); // the coordinates have to be valid else an assert fires.
    
    // retrieve a pointer to the neighbour hex found in the indicated direction.
    // if that neighbour is out of ranges of the map a NULL value is returned.
    Hexagon* getHex(Hexagon& hex, Direction dir) const;
    
    // determine the coordinates of the vector (h1,h2) and return its Coordl.
    Coordl getVect(Hexagon& h1, Hexagon& h2) const; 
    
    // get the coordinates of the given hexagon.
    Coordl getCoordL(Hexagon& hex) const {return hex.coord.l;};
    
    // Addition of an hexagon and a vector. If the operation is impossible (out
    // of limits) then the returned value is null, else the coordinates of the
    // hex are returned..
    Coordl getCoordL(Hexagon& h, const HexCoord& vect) const;
   
    Coordl getCoordL(Hexagon& h, HexCoord& vect) const 
    {
        const HexCoord& cvect = vect; 
        return getCoordL(h, cvect);
    };

    // compute the distance between two hexagons.
    // returns a negative value if one of them is out of limits
    int hexDist(Hexagon& h1, Hexagon& h2); 


};


};

// MODULE TESTS
extern int test1_Hexmap_hexDist(void);

#endif /* HEXMAP_H */

