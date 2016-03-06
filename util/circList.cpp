/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "circList.h"
#include "assert.h"
using namespace std;

template <typename T_> 
T_* circular_list::
first()
{
        assert(this->container == NULL);// list head.
        if (!empty())
        {
            return this->next;
        }
        else
            return NULL;
    
}
template <typename T_> 
T_* circular_list::
last ()
{
        assert(this->container == NULL);// list head.
        if (!empty())
        {
            return this->prev;
        }
        else
            return NULL;
    
}



template <typename T_> 
void circular_list::
insert_after(circular_list<T_> *elt)
{
        assert(this->container != NULL); // cannot be a list head.
        assert(elt->container != NULL); // cannot be a list head.

        __insert(elt, elt->next);
}

template <typename T_> 
void circular_list::
insert_before(circular_list<T_> *elt)
{
        assert(this->container != NULL);// cannot be a list head.
        assert(elt->container != NULL); // cannot be a list head.
        
        __insert(elt->prev, elt);
}


template <typename T_> 
void circular_list::
push(circular_list<T_> *newElt)
{
    assert(this->container == NULL);
    assert(newElt->container != NULL);
    
    newElt->__insert(this, next);
    
}

template <typename T_> 
void circular_list::
push_back(circular_list<T_> *newElt)
{
    assert(this->container == NULL);
    assert(newElt->container != NULL);
    
    newElt->__insert(this->prev, this);
    
}

template <typename T_> 
T_* circular_list::
extract()
{
        assert(this->container != NULL);// cannot be a list head.
        if (!empty())
        {
            __remove(this->prev, this->next);
            this->init();
        }
        return this->payload;
}

template <typename T_> 
circular_list<T_>* circular_list::
pop()
{
        assert(this->container == NULL);// list head.
        if (!empty())
        {
            circular_list<T_>*first = this->next;
            __remove(this, first->next);
            first->init();
            return first;
        }
        else
            return NULL;
    
}
template <typename T_> 
T_* circular_list::
pop()
{
        circular_list<T_>* first = pop();
        
        if (first != NULL)
        {
            return first->payload;
        }
        else
            return NULL;
}

template <typename T_> 
circular_list<T_>* circular_list::
pop_back()
{
        assert(this->container == NULL);// list head.
        if (!empty())
        {
            circular_list<T_>*last = this->prev;
            __remove(last->prev, this);
            last->init();
            return last;
        }
        else
            return NULL;
    
}
template <typename T_> 
T_* circular_list::
pop_back()
{
        circular_list<T_>* first = pop();
        
        if (first != NULL)
        {
            return first->payload;
        }
        else
            return NULL;
}


template <typename T_> 
void circular_list::
replace(circular_list<T_> *elt)
{
        assert(this->container != NULL);
        assert(elt->container != NULL);
        this->next = elt->next;
        this->next->prev = this;
        this->prev = elt->prev;
        this->prev->next = this;
        elt->init();
}
/**
 * replace_by - replace old entry by new one
 * @this : the element to be replaced. Can be a list head or not.
 * @newElt : the new element to insert.  Can be a list head or not.
 *
 */
template <typename T_> 
void circular_list::
replace_by(circular_list<T_> *newElt)
{
        // both elements shall have the same natur (list head or payload element.)
        assert(this->payload == newElt->payload);
        if (empty())
            return;
        newElt->next = this->next;
        newElt->next->prev = newElt;
        newElt->prev = this->prev;
        newElt->prev->next = newElt;
        this->init();
}

/**
 * move - remove the current element from its list and push it into another one
 * @this: the entry to move
 * @head: the head of the new list
 */
 template <typename T_> 
 void circular_list::
 move_elt(circular_list<T_> *head)
{
        assert(head->payload == NULL);
        assert(this->payload != NULL);
        __remove(this->prev, this->next);
        head->push(this)
}

/**
 * move_tail - delete from one list and add as another's tail
 * @head: the head that will follow our entry
 */
template <typename T_> 
void circular_list::
move_elt_tail(circular_list<T_> *head)
{
        assert(head->payload == NULL);
        assert(this->payload != NULL);
        
        __remove(this->prev, this->next);
        head->push_back(this)
}

/**
 * is_last - tests whether the object is the last entry in list @head
 * @this: the entry to test
 * @head: the head of the list
 */
template <typename T_> 
bool circular_list::
is_last(const circular_list<T_> *head)
{
        assert(head->payload == NULL);
        assert(this->payload != NULL);

        return next == head;
}

/**
 * rotate_left - rotate the list to the left: the next element is moved to the tail.
 * @this: the head of the list. An assert is firing if not a list's head.
 */
template <typename T_> 
void circular_list::
rotate_left()
{
        assert(payload == NULL);
        if (!empty()) {
                next->move_elt_tail(this);
        }
}

template <typename T_> 
int circular_list::
is_singular()
{
        assert(payload == NULL);
        return !empty() && (next == prev);
}

template <typename T_> 
void circular_list::
truncate(circular_list<T_> *entry,
         circular_list<T_> *list )
{
        assert(payload == NULL);
        assert(list->payload == NULL);
        assert(entry->payload!=NULL);
        if (empty())
                return;
        if (is_singular() &&
            (this->next != entry && this != entry))
            return;
        if (entry == this)
            list->init();
        else
        {
            circular_list<T_> *new_first = entry->next;
            list->next = this->next;
            list->next->prev = list;
            list->prev = entry;
            entry->next = list;
            this->next = new_first;
            new_first->prev = this;
        }
}

template <typename T_> 
void circular_list::
append(const circular_list<T_> *list)
{
        assert(list->payload == NULL);
        assert(payload == NULL);

	if (!list->empty())
		__concat(list, this, this->next);
}

template <typename T_> 
void circular_list:: 
append_back(circular_list<T_> *list)
{
        assert(list->payload == NULL);
        assert(payload == NULL);
        
        if (!list->empty())
                __concat(list, this->prev, this);
}
