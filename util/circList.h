/* 
 * File:   circular_list.h
 * Author: jlaclavere
 *
 * Created on March 5, 2016, 5:32 PM
 */

#ifndef DLINK_H
#define DLINK_H

#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H
#include <stddef.h>

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define POISON_POINTER_DELTA 0 // 0xdead000000000000 for 64 bits, etc...
#define LIST_POISON1  ((void *) 0x00100100 + POISON_POINTER_DELTA)
#define LIST_POISON2  ((void *) 0x00200200 + POISON_POINTER_DELTA)

//#include <linux/const.h>

/*
 * Simple doubly linked list implementation derivated from the kernel's list.h
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */
template <typename T_>
struct circular_list
{
    
private:
    struct circular_list<T_> *next, prev;
    
    /* the offsetof() macro that is used in C to retrieve the address of the 
     * containing struct cannot be used easily in C++. The class cannot use
     * virtual method in particular - 
     * see http://www.cplusplus.com/reference/type_traits/is_standard_layout/
     * So we introduce a third member, container, keeping the this of the englobing
     * object. Since this field has only meaning with the elements of the list,
     * it can be kept null for the list's head, allowing some level of control.
     */
    T_ *payload; // NULL for the list's head. The onwner object if an element of the list.
    
    circular_list(T_ *listEltOf) {payload = listEltOf; next = this; prev = this;};
    init(){next = this; prev = this;};

    /*
     * Insert the object between two known consecutive entries.
     * the object cannot be the list head else an assert is firing.
     * This is only for internal list manipulation where we know
     * the prev/next entries already!
     */
    void __insert(circular_list<T_> *prev,
                  circular_list<T_> *next)
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
    __remove(circular_list<T_> * prev, 
             circular_list<T_> * next)
    {
        next->prev = prev;
        prev->next = next;
    }

    inline static void 
    __concat(const circular_list<T_> *list,
                        circular_list<T_> *prev,
                        circular_list<T_> *next)
    {
            circular_list<T_> *first = list->next;
            circular_list<T_> *last = list->prev;

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
    T_*  object() {return payload;}
    
    /**
     * empty - tests wether the object forms an empty list.
     * Normaly used on the list's head, but may also be used on
     * an element to test whether it belongs to a list or not.
     */
    bool empty() { return next == this; }

    T_*  first();
    T_*  last ();
    
    /**
     * insert_after - insert the object right after the indicated item
     * @this: new entry to be added. must be list element (not a list head).
     * @elt: reference element. Can be a list's head or not.
     * Insert a new entry after the specified element.
     */
    void insert_after(circular_list<T_> *elt);
    
    /**
    * insert_before. insert the object right before the indicated item
    * @this: new entry to be added. must be list element (not a list head)
    * @elt: reference element. Cannot be a list head.
    *
    * Insert a new entry before the specified item, so at the tail of the list for a list head.
    */
   void insert_before(circular_list<T_> *elt);

    /**
     * push - push the indicated element to the head of the list
     * @newElt: list head to add it after
     *
     * Insert a new entry at the first place
     * This is good for implementing stacks.
     */
    void push(circular_list<T_> *newElt);
    
    /**
     * push_back - push the indicated element to the end of the list
     * @newElt: list head 
     *
     * Insert a new entry at the tail of the list
     * This is good for implementing queues.
     */
    void push_back(circular_list<T_> *newElt);
    
    /**
     * extract - deletes the object from its list and reinitialize it.
     * @this: a list element. Cannot be a list head.
     * @return the pointer to the queued object.
     */
    T_* extract();

    
    /**
     * pop - first prototype.
     * Extract the first element from the list and returns its address.
     * @this: a list head, else assert. If empty, NULL is returned.
     * @return the pointer to the extracted element.
     * This is good for implementing stacks.
     */
    circular_list<T_>* pop();
    
    /**
     * pop - second prototype. Like the first one but the pointer to the 
     * englobing object is returned instead.
     */
    
    T_*   pop();
    
    /**
     * pop_back - first prototype.
     * Extract the first element from the list and returns its address.
     * @this: a list head, else assert. If empty, NULL is returned.
     * @return the pointer to the extracted element.
     * This is good for implementing queues.
     */
    circular_list<T_>* pop_back();
    
    /**
     * pop_back - second prototype. Like the first one but the pointer to the 
     * englobing object is returned instead.
     */
    
    T_*   pop_back();

    /**
     * replace - replace old entry by new one
     * @old : the element to be replaced. Cannot be a list's head.
     * @this : the new element to insert. Cannot be a list's head.
     *
     * If @old was empty, it will be overwritten.
     */
    void replace(circular_list<T_> *old);
    /**
     * replace_by - replace old entry by new one
     * @this : the element to be replaced
     * @newElt : the new element to insert
     *
     * If @old was empty, it will be overwritten.
     */
    void replace_by(circular_list<T_> *newElt);
    /**
     * move - delete from one list and add as another's head
     * @this: the entry to move
     * @head: the head that will precede our entry
     */
     void move_elt(circular_list<T_> *head);

    /**
     * move_tail - delete from one list and add as another's tail
     * @head: the head that will follow our entry
     */
    void move_elt_tail(circular_list<T_> *head);

    /**
     * is_last - tests whether the object is the last entry in list @head
     * @this: the entry to test
     * @head: the head of the list
     */
    bool is_last(const circular_list<T_> *head);

    /**
     * rotate_left - rotate the list to the left. the next element is moved to the tail.
     * @this: the head of the list
     */
    void rotate_left();
    /**
     * is_singular - tests whether the list has just one entry.
     * @head: the list to test. Assert firing if not a list's head.
     */
    int is_singular();

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
    void truncate(circular_list<T_> *entry, 
                  circular_list<T_> *list );

    /**
     * append - insert the given list at the head of the current list.
     * @this: Must be a list head. 
     * @list: the new list to add.  Must be a list head. The list head itself is not queued.
     */
    void append(const circular_list<T_> *list);

    /**
     * append_back - insert the given list at the tail of the current list.
     * @this: Must be a list head.  
     * @list: the new list to add.
     */
    void append_back(circular_list<T_> *list);

};


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


#endif /* DLINK_H */

