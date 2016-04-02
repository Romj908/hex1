/* 
 * File:   circular_list.h
 * Author: jlaclavere
 *
 * Created on March 5, 2016, 5:32 PM
 */

#ifndef DLINK_H
#define DLINK_H

#include <stddef.h>
#include "assert.h"
#include <iterator>
#include <utility>
#include <iostream>
namespace CircList {

/*
 * Efficient doubly linked list implementation derivated from the kernel's list.h
 * the circular_list<T_> template 
 */

template <typename T_>
class circular_list_elt
{
public:
    
    // Attribute fields.
    
    struct circular_list_elt<T_> *next;
    struct circular_list_elt<T_> *prev;
    
    /* the offsetof() macro that is used in C to retrieve the address of the 
     * containing struct cannot be used easily in C++. The class cannot use
     * virtual method in particular - 
     * see http://www.cplusplus.com/reference/type_traits/is_standard_layout/
     * So we introduce a third member, payload, keeping the this of the englobing
     * object. Since this field has only meaning with the elements of the list,
     * it can be kept null for the list's head, allowing some level of control.
     */
private:        // too dangerous to be made it public. It can never be modified.
    T_ *payload; // NULL for the list's head. The onwner object for an element of the list.

public:    
    circular_list_elt(T_ *listEltOf) {payload = listEltOf; next = this; prev = this;};

//protected: this encapsulation by protect is (unfortunately) not possible due to
// the restriction to this encapsulation type : "A protected member of a class Base can only 
// be accessed... by the members ... of any class derived from Base, but only when operating 
// on an object of a type that is derived from Base (including this)."
    void __init(){next = this; prev = this;};  
    
    /*
     * Insert the object between two known consecutive entries.
     * This is only for internal list manipulation where we know
     * the prev/next entries already!
     */
    void __insert(circular_list_elt<T_> *prev,
                  circular_list_elt<T_> *next)
    {
        next->prev = this;
        this->next = next;
        this->prev = prev;
        prev->next = this;
    }

    /*
     * Delete a list entry by making the prev/next entries
     * point to each other.
     *
     * This is only for internal list manipulation where we know
     * the prev/next entries already!
     */
    inline static void 
    __link(circular_list_elt<T_> * prev, 
             circular_list_elt<T_> * next)
    {
        next->prev = prev;
        prev->next = next;
    }

    inline static void 
    __concat(const circular_list_elt<T_> *list,
                        circular_list_elt<T_> *prev,
                        circular_list_elt<T_> *next)
    {
            circular_list_elt<T_> *first = list->next;
            circular_list_elt<T_> *last = list->prev;

            first->prev = prev;
            prev->next = first;

            last->next = next;
            next->prev = last;
    }


public:
    /* test is the object is the head of the list. That function should rarely be used.*/
    bool is_head() {return payload == NULL;}
    
    /* get the pointer to the payload object of that list element. 
       Be careful : when used on the list's head the returned value is NULL */
    T_*  get_payload() const {return payload;}
    
    /**
     * empty - tests wether the object forms an empty list.
     * Normaly used on the list's head, but may also be used on
     * an element to test whether it belongs to a list or not.
     */
    bool empty() { return next == this; }

    /* get the pointer to the first element of the list without extracting it.*/
    T_*  first()
    {
        assert(this->payload == NULL);// list head.
        if (!empty())
        {
            return this->next->get_payload();
        }
        else
            return NULL;
    }

    /* get the pointer to the last element of the list without extracting it.*/
    T_* 
    last ()
    {
            assert(this->payload == NULL);// list head.
            if (!empty())
            {
                return this->prev->get_payload();
            }
            else
                return NULL;
    }

    /**
     * insert_after - insert the object right after the indicated item
     * @this: new entry to be added. must be list element (not a list head).
     * @elt: reference element. Can be a list's head or not.
     * Insert a new entry after the specified element.
     */
    void 
    insert_after(circular_list_elt<T_> *elt)
    {
            assert(this->payload != NULL); // cannot be a list head.
            __insert(elt, elt->next);
    }
    
    /**
    * insert_before. insert the object right before the indicated item
    * @this: new entry to be added. must be list element (not a list head)
    * @elt: reference element. Cannot be a list head.
    *
    * Insert a new entry before the specified item, so at the tail of the list for a list head.
    */
    void 
    insert_before(circular_list_elt<T_> *elt)
    {
            assert(this->payload != NULL);// cannot be a list head.
            __insert(elt->prev, elt);
    }

    /**
     * extract - deletes the object from its list and reinitialize it.
     * @this: a list element. Cannot be a list head.
     * @return the pointer to the queued object.
     */
    T_* extract();
  
    /**
     * replace - replace old entry by new one
     * @old : the element to be replaced. Cannot be a list's head.
     * @this : the new element to insert. Cannot be a list's head.
     *
     * If @old was empty, it will be overwritten.
     */
    void 
    replace(circular_list_elt<T_> *elt);

    /**
     * replace_by - replace old entry by new one
     * @this : the element to be replaced. Can be a list head or not.
     * @newElt : the new element to insert.  Can be a list head or not.
     */
    void 
    replace_by(circular_list_elt<T_> *newElt);

    /**
     * move - remove the current element from its list and push it into another one
     * @this: the entry to move
     * @target: any element in a list. Not necerrary the head of a list
     */
    void 
    move_elt(circular_list_elt<T_> *target);

    /**
     * move_tail - delete from one list and add as another's tail
     * @target: any element in a list. Not necerrary the head of a list
     */
    void 
    move_elt_tail(circular_list_elt<T_> *target);
    /**
     * is_last - tests whether the object is the last entry in list @head
     * @this: the entry to test
     * @head: the head of the list
     */
    bool 
    is_last(const circular_list_elt<T_> *head)
    {
            assert(head->payload == NULL);
            assert(this->payload != NULL);
            return next == head;
    }

    /**
    * rotate_left - rotate the list to the left: the next element is moved to the tail.
    * @this: the head of the list. An assert is firing if not a list's head.
    */
   void 
   rotate_left();

};
 
template <typename T_>
T_* circular_list_elt<T_>::
extract()
{
        assert(this->payload != NULL);// cannot be a list head.
        if (!empty())
        {
            __link(this->prev, this->next);
            this->__init();
        }
        return this->payload;
}



template <typename T_>
void circular_list_elt<T_>::
replace(circular_list_elt<T_> *elt)
{
        assert(this->payload != NULL);
        assert(elt->payload != NULL);
        this->next = elt->next;
        this->next->prev = this;
        this->prev = elt->prev;
        this->prev->next = this;
        elt->__init();
}

template <typename T_>
void circular_list_elt<T_>::
replace_by(circular_list_elt<T_> *newElt)
{
        // both elements shall have the same natur (list head or payload element.)
        assert(this->payload == newElt->get_payload());
        if (empty())
            return;
        newElt->next = this->next;
        newElt->next->prev = newElt;
        newElt->prev = this->prev;
        newElt->prev->next = newElt;
        this->__init();
}

template <typename T_>
void circular_list_elt<T_>::
move_elt(circular_list_elt<T_> *target)
{
       assert(this->payload != NULL);
       if (!empty())
       {
            // extract the element from it's list.
            __link(this->prev, this->next);
       }
       // insert this just after the target element.
       __insert(target, target->next);
}


template <typename T_>
void circular_list_elt<T_>::
move_elt_tail(circular_list_elt<T_> *target)
{
        assert(this->payload != NULL);
       if (!empty())
       {
            // extract the element from it's list.
            __link(this->prev, this->next);
       }
       // insert this just before the target element.
       __insert(target->prev, target);
}

template <typename T_>
void circular_list_elt<T_>::
rotate_left()
{
        assert(payload == NULL);
        if (!empty()) {
                next->move_elt_tail(this);
        }
}

/**
 * The list's head is an instance of the circular_list_head<T_> class inherited 
 * from circular_list<T_>
 * One such instance always points to the first element of the list of to itself
 * when the list is empty.
 */
// forward declaration of the iterator class because it is required by the 
// definition of circular_list::begin() and circular_list::end()
template <typename PAYLOAD>
class CircListIterator;

template <typename T_>
class circular_list_head;

template <typename T_>
class circular_list_head : public circular_list_elt<T_>
{
    public:
        typedef CircListIterator<T_> iterator;
        typedef const CircListIterator<T_> const_iterator;
        
        circular_list_head() : circular_list_elt<T_>(nullptr) {};
        
        // forbid any global copy:
        circular_list_head(circular_list_head<T_> & ) = delete;
        circular_list_head<T_>& operator= (circular_list_head<T_> &) = delete;
        
        // don't make the destructor virtual: this class is final and we want to
        // spare space by not having any __vfptr pointer in the circular_list<T_>.
        ~circular_list_head() 
        {
            // check that the list is well empty before to be deleted.
            // We don't delete the objects in the list because they could be linked in
            // several different lists (have several attribute instance of circular_list<T_>)
            // So all the linked objects have to be extracted from the list before to destroy it.
             assert(this->empty());
        }
    protected:
//        inline void 
//        __for_each()
//        {
//            circular_list<T_> *p;
//            for (p = this->next; p != this; p = p->next)
//            {
//            }
//        }
        
public:
/* get the pointer to the first element of the list without extracting it.*/
    T_*  front()
    {
        assert(this->get_payload() == NULL);// list head.
        if (!this->empty())
        {
            return this->next->get_payload();
        }
        else
            return NULL;
    }

    /* get the pointer to the last element of the list without extracting it.*/
    T_*  back ()
    {
            assert(this->get_payload() == NULL);// list head.
            if (!this->empty())
            {
                return this->prev->get_payload();
            }
            else
                return NULL;
    }
    inline T_*  first() {return this->front();};
    inline T_*  last()  {return this->back();};

    /**
     * begin() standart function to start a list traversal (used with iterators)
     * The this must be a list head.
     * @return 
     */
    iterator begin()
    {
        assert(this->get_payload() == NULL);// list head.
        iterator it(this->next);
        return it;
    }

    const_iterator cbegin() const
    {
        assert(this->get_payload() == NULL);// list head.
        const_iterator it(this->next);
        return it;
    }
    
   /**
     * rbegin() standart function to start a list traversal in reverse order
     * The this must be a list head.
     * @return 
     */
    iterator rbegin()
    {
        assert(this->get_payload() == NULL);// list head.
        iterator it(this->prev);
        return it;
    }

    const_iterator crbegin() const
    {
        assert(this->get_payload() == NULL);// list head.
        const_iterator it(this->prev);
        return it;
    }

    /**
     * end() standart function to end a list traversal (used with iterators)
     * The this must be a list head.
     * @return the pointer when all the list has been scanned. It's the this.
     */
    iterator end ()
    {
            assert(this->get_payload() == NULL);// list head.
            iterator it(this);
            return it;
    }

    /**
     * end() standart function to end a list traversal (used with iterators)
     * The this must be a list head.
     * @return the pointer when all the list has been scanned in reverse order. 
     * It's the this.
     */
    iterator rend ()
    {
        return end();
    }

    const_iterator cend () const
    {
            assert(this->get_payload() == NULL);// list head.
            const_iterator it(this);
            return it;
    }
    
    const_iterator crend () const
    {
            assert(this->get_payload() == NULL);// list head.
            const_iterator it(this);
            return it;
    }
    
   /**
     * is_singular - tests whether the list has just one entry.
     * @head: the list to test. Assert firing if not a list's head.
     */
    bool 
    is_singular() 
    {
            assert(this->get_payload() == NULL);
            return !this->empty() && (this->next == this->prev);
    }

    /**
     * push - push the indicated element to the head of the list
     * @newElt: list head to add it after
     *
     * Insert a new entry at the first place
     * This is good for implementing stacks.
     */
    void 
    push(circular_list_elt<T_> *newElt);

    /**
     * push_back - push the indicated element to the end of the list
     * @newElt: list head 
     *
     * Insert a new entry at the tail of the list
     * This is good for implementing queues.
     */
    void 
    push_back(circular_list_elt<T_> *newElt);
    
    /**
     * pop - first prototype.
     * Extract the first element from the list and returns its address.
     * @this: a list head, else assert. If empty, NULL is returned.
     * @return the pointer to the extracted element.
     * This is good for implementing stacks.
     */
    circular_list_elt<T_>* 
    pop();
    
    /**
     * pop_back - first prototype.
     * Extract the first element from the list and returns its address.
     * @this: a list head, else assert. If empty, NULL is returned.
     * @return the pointer to the extracted element.
     * This is good for implementing queues.
     */
    circular_list_elt<T_>* 
    pop_back();
    
    /**
     * truncate - cut a list into two
     * @this: a list with entries. Assert firing if not a list's head.
     * @entry: an entry within this, could be the head itself
     *	and if so we won't cut the list. Assert firing if it's a list's head.
     * @list: a new list to add all removed entries. Assert firing if not a list's head.
     *
     * This helper moves the initial part of @this, up to and
     * including @entry, from @this to @list. You should
     * pass on @entry an element you know is on @this. @list
     * should be an empty list or a list you do not care about
     * losing its data.
     *
     */
    void 
    truncate(circular_list_elt<T_> *entry,
             circular_list_elt<T_> *list );

    /**
     * append - insert the given list at the head of the current list.
     * @this: Must be a list head. 
     * @list: the new list to add.  Must be a list head. The list head itself is not queued.
     */
    void 
    append(const circular_list_elt<T_> *list)
    {
            assert(list->get_payload() == NULL);
            assert(this->get_payload() == NULL);
            if (!list->empty())
                    __concat(list, this, this->next);
    }

    /**
     * append_back - insert the given list at the tail of the current list.
     * @this: Must be a list head.  
     * @list: the new list to add.
     */
    void  
    append_back(circular_list_elt<T_> *list)
    {
            assert(list->get_payload() == NULL);
            assert(this->get_payload() == NULL);
            if (!list->empty())
                    __concat(list, this->prev, this);
    }

}; // circular_list_head

template <typename T_>
void circular_list_head<T_>::
push(circular_list_elt<T_> *newElt)
{
    assert(this->get_payload() == NULL);
    assert(newElt->get_payload() != NULL);
    newElt->__insert(this, this->next);
    
}

template <typename T_>
void circular_list_head<T_>::
push_back(circular_list_elt<T_> *newElt)
{
    assert(this->get_payload() == NULL);
    assert(newElt->get_payload() != NULL);
    newElt->__insert(this->prev, this);
}


template <typename T_>
circular_list_elt<T_>* circular_list_head<T_>::
pop()
{
    assert(this->get_payload() == NULL);// list head.
    if (!this->empty())
    {
        circular_list_elt<T_>*first = this->next;
        __link(this, first->next);
        first->__init();
        return first;
    }
    else
        return NULL;
}

template <typename T_>
circular_list_elt<T_>* circular_list_head<T_>::
pop_back()
{
        assert(this->get_payload() == NULL);// list head.
        if (!this->empty())
        {
            circular_list_elt<T_>*last = this->prev;
            this->__link(last->prev, this);
            last->__init();
            return last;
        }
        else
            return NULL;
}

template <typename T_> 
void circular_list_head<T_>::
truncate(circular_list_elt<T_> *entry,
         circular_list_elt<T_> *list )
{
        assert(this->get_payload() == NULL);
        assert(list->get_payload() == NULL);
        assert(entry->get_payload()!=NULL);
        if (this->empty())
                return;
        if (is_singular() &&
            (this->next != entry && this != entry))
            return;
        if (entry == this)
            list->__init();
        else
        {
            circular_list_elt<T_> *new_first = entry->next;
            list->next = this->next;
            list->next->prev = list;
            list->prev = entry;
            entry->next = list;
            this->next = new_first;
            new_first->prev = this;
        }
}


/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct circular_list to use as a loop cursor.
 * @head:	The pointer to the list head element.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct circular_list to use as a loop cursor.
 * @head:	The pointer to the list head element.
 */
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct circular_list to use as a loop cursor.
 * @n:		another &struct circular_list to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct circular_list to use as a loop cursor.
 * @n:		another &struct circular_list to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)



/**
 * Definition of the circular_list's iterator
 * Our iterator's value-type must be of course the same "PAYLOAD" class than in
 * circular_list<PAYLOAD> because it describes the type of the payload objects.
 */

//// Forward declarations to allow friend templates.
//// refer to : https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Making_New_Friends
//
//template<class T> 
//class CircListIterator;
//
//template<class T> 
//void std::swap (CircListIterator<T>& a, CircListIterator<T>& b);


template <typename PAYLOAD>
class CircListIterator : public std::iterator<std::bidirectional_iterator_tag, 
                                                circular_list_elt<PAYLOAD>>
{
    public:
  circular_list_elt<PAYLOAD>* p_elt;
  circular_list_elt<PAYLOAD>* p_prev;
  circular_list_elt<PAYLOAD>* p_next;
  
  void __relink(circular_list_elt<PAYLOAD>* x)
  {
      p_elt = x; 
      p_prev = x->prev; 
      p_next = x->next;
  }
  
  void __safe()
  {
      p_prev = p_elt->prev; 
      p_next = p_elt->next;
  }
  
public:
  
  CircListIterator(circular_list_elt<PAYLOAD>* x = nullptr) 
                : p_elt(x), 
                  p_prev(x!=nullptr ? x->prev : x), 
                  p_next(x!=nullptr ? x->next : x) 
                {}
  CircListIterator(const CircListIterator& iter) = default;

  virtual ~CircListIterator() {};
  
  circular_list_elt<PAYLOAD>* get() {return p_elt; };
  
  CircListIterator& operator++() 
  {
      
      //move to the next element without using the contents of p_elt which may have been destroyed.
      p_prev = p_elt;
      p_elt = p_next;
      p_next = p_next->next;
      
      return *this;
  }
  
  CircListIterator operator++(int) 
  {
      CircListIterator tmp(*this); 
      operator++(); 
      return tmp;
  }
  
  CircListIterator& operator--() 
  {
        //move to the previous element without using the contents of p_elt which may have been destroyed.
        p_next = p_elt; 
        p_elt = p_prev; 
        p_prev = p_prev->prev;
      
        return *this;
  }
  
  CircListIterator operator--(int) 
  {
      CircListIterator tmp(*this); 
      operator--(); 
      return tmp;
  }
  
  
  bool operator==(const CircListIterator& iter) {return p_elt==iter.p_elt;}
  bool operator!=(const CircListIterator& iter) {return p_elt!=iter.p_elt;}
  
  PAYLOAD& operator*() 
  {
      assert(p_elt->get_payload());
      return *(p_elt->get_payload());
  }
  
    // template swap() function required for complete support of the iterator.
    friend
    void swap (CircListIterator<PAYLOAD>& a, CircListIterator<PAYLOAD>& b)
       {
        // T c(std::move(a)); a=std::move(b); b=std::move(c);
        CircList::circular_list_elt<PAYLOAD>* elt_a = a.p_elt;
        CircList::circular_list_elt<PAYLOAD>* elt_b = b.p_elt;
        // recompute the links NOW. This is because external iterators used after 
        //a swap() could have uncorrect next/prev links.
        a.__safe();
        b.__safe();
        std::cout << "\nswap() of " << elt_a->get_payload() << " with " << elt_b->get_payload();
        elt_a->extract();

        if (a.p_next == elt_b)
        {
            // a was right before b
            assert(b.p_prev == elt_a);
            elt_a->insert_after(elt_b);
        }
        else if (a.p_prev == elt_b)
        {
            // a was right after b
            assert(b.p_next == elt_a);
            elt_a->insert_before(elt_b);
        }
        else
        {
            // a and b were not adjacent
            elt_b->extract();
            elt_a->insert_after(b.p_prev);
            elt_b->insert_after(a.p_prev);
        }
        // update both iterators.
        a.__relink(elt_b);
        b.__relink(elt_a);
    }
    
// Assuming that it's not necessary to redefine the array variant of the swap template.    
//    template <class T, size_t N> 
//    void swap (T &a[N], T &b[N])
//    {
//      for (size_t i = 0; i<N; ++i) swap (a[i],b[i]);
//    }

};

}; // namespace

#endif /* DLINK_H */

