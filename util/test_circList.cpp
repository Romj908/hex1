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
using namespace CircList;

class FooLinkableObject;

class FooLinkableObject
{
public:
    int identity;
    
    circular_list<FooLinkableObject> list_elt;
    // constructors
    FooLinkableObject() : list_elt(this) {}
    FooLinkableObject(int id): identity(id), list_elt(this) {};
    virtual ~FooLinkableObject() {};
    
    friend ostream& operator<<(ostream& o, FooLinkableObject &obj);
    friend ostream& operator<<(ostream& o, FooLinkableObject *obj);
    
};
ostream& operator<<(ostream& o, FooLinkableObject& obj)
{
    o << "FooLinkableObject id :" << obj.identity;
    return o;
}

ostream& operator<<(ostream& o, FooLinkableObject *obj)
{
    o << "FooLinkableObject id :" << obj->identity;
    return o;
}


class FooMasterObject
{
public:
    int dummy;
    circular_list_head<FooLinkableObject> the_list;
    FooMasterObject() :the_list() {}
    virtual ~FooMasterObject() {};
    
    void disp_list();
};

void FooMasterObject::disp_list()
{
    circular_list<FooLinkableObject> *p;
    cout << "\n";
    for (FooLinkableObject & obj : the_list)
    {
        cout << obj << endl;
    }
}
/* test of the iterators and of the std::swap() function */
int test1_circList(void)
{
    FooMasterObject master;
    cout << "\n master list elts 0 to 2 ";
    for (int i=0; i<3; i++)
    {
        FooLinkableObject *obj = new FooLinkableObject(i);
        master.the_list.push_back( &(obj->list_elt) ); 
    }
    master.disp_list(); 
    CircListIterator<FooLinkableObject> it_beg(master.the_list.begin());
    CircListIterator<FooLinkableObject> it_end(master.the_list.end());
    it_end--;
    CircListIterator<FooLinkableObject> it_middle = it_beg;
    it_middle++;
    
    swap(it_beg, it_end);
    master.disp_list(); 
    
    swap(it_beg, it_middle);
    master.disp_list(); 
    
    swap(it_beg, it_middle);
    master.disp_list(); 
    
    swap(it_middle, it_end);
    master.disp_list(); 
    
    swap(it_middle, it_end);
    master.disp_list(); 
    
    swap(it_end, it_middle);
    master.disp_list(); 
    
    swap(it_end, it_middle);
    master.disp_list(); 
    
    
    cout << "\n remove the middle elt and swap elts 1 and 0 ";
    it_middle.get()->extract();
    delete it_middle.get()->get_payload();
    
    swap(it_beg, it_end);
    master.disp_list(); 
    
    swap(it_beg, it_end);
    master.disp_list(); 
    
    for (FooLinkableObject & obj : master.the_list)
    {
        cout << "\ndeleting " << obj;
        obj.list_elt.extract();
        delete &obj;
    }
    return 0;
}

#if 1
/* Use of the circular_list's iterator */
int test2_circList(void)
{
    FooMasterObject master;
    FooMasterObject master2;
    circular_list<FooLinkableObject> *p;
    cout << "\n test2_circList ";
    
    cout << "\n master list elts 9 to 0 ";
    for (int i=0; i<10; i++)
    {
        FooLinkableObject *obj = new FooLinkableObject(i);
        master.the_list.push( &(obj->list_elt) ); 
    }
    master.disp_list(); 
    
    assert((master.the_list.first())->identity == 9);
    assert((master.the_list.last())->identity == 0);
    cout << "\n de-queing master's elts and queing them to master2 ";
    
    while (!master.the_list.empty())
    {
        p = master.the_list.pop_back();
        master2.the_list.push_back(p);
    }
    master.disp_list(); 
    assert(master.the_list.empty());
    master2.disp_list(); 
    assert((master2.the_list.first())->identity == 0);
    assert((master2.the_list.last())->identity == 9);
    
    cout << "\n CircListIterator on master2 with iterator :\n";
    ;
    CircListIterator<FooLinkableObject> it;
    for (it = master2.the_list.begin() ; it != master2.the_list.end(); it++)
    {
        cout << *it << endl;
    }
    
    cout << "\n for (FooLinkableObject & obj : master2.the_list) :\n";

    for (FooLinkableObject & obj : master2.the_list)
    {
        cout << obj << endl;        
    }
    cout << "\n test of the circular_list::trucation. move the master2 elements [0..3] to master\n ";
    circular_list<FooLinkableObject> *p3;

    for (FooLinkableObject & obj : master2.the_list)
    {
        if (obj.identity == 3)
        {
            p3 = &obj.list_elt;
            break;
        }
    }
    master2.the_list.truncate(p3, &master.the_list);
    
    cout << "\n master2:\n ";
    master2.disp_list(); 

    cout << "\n master:\n ";
    master.disp_list(); 
    
    assert((master.the_list.first())->identity == 0);
    assert((master.the_list.last())->identity == 3);
    
    assert((master2.the_list.first())->identity == 4);
    assert((master2.the_list.last())->identity == 9);
    
    while (!master.the_list.empty())
    {
        p = master.the_list.pop_back();
        cout << "\ndeleting " << *(p->get_payload());
        delete p->get_payload();
    }
    master.disp_list(); 
    
    for (FooLinkableObject & obj : master2.the_list)
    {
        cout << "\ndeleting " << obj;
        obj.list_elt.extract();
        delete &obj;
    }
    master2.disp_list(); 
    
    return 0;
}
#endif