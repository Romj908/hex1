/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "circList.h"

#include <cstdlib>
#include <iostream>

#include "assert.h"
using namespace std;

class FooLinkableObject;

class FooLinkableObject
{
public:
    int identity;
    circular_list<FooLinkableObject> list_elt;
    FooLinkableObject() : list_elt(this) {}
    
    FooLinkableObject(int id): identity(id), list_elt(this) {};
    friend ostream& operator<<(ostream& o, FooLinkableObject & obj);
    
};
ostream& operator<<(ostream& o, FooLinkableObject & obj)
{
    o << "FooLinkableObject id " << obj.identity;
    return o;
}


class FooMasterObject
{
public:
    int dummy;
    circular_list<FooLinkableObject> the_list;
    FooMasterObject() :the_list(NULL) {}
    void disp_list();
};

void FooMasterObject::disp_list()
{
    circular_list<FooLinkableObject> *p;
    cout << "\n";
    list_for_each(p, &the_list)
    {
        cout << p << endl;
    }
}
int test1_circList(void)
{
    FooMasterObject master;
    
    for (int i=0; i<10; i++)
    {
        FooLinkableObject *obj = new FooLinkableObject(i);
        master.the_list.push( &(obj->list_elt) ); 
    }
    master.disp_list();
    
}