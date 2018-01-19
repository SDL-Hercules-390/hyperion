/* LINKLIST.H   (C) Copyright Roger Bowler, 2006-2012                */
/*              linked-list macros                                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _LLIST_
#define _LLIST_

//////////////////////////////////////////////////////////////////////////////////////////
/*
    This module is a standalone collection of linked-list definition,
    and manipulation macros originally defined for Windows NT development.


 Samples:


    //  Define a list head.

    LIST_ENTRY  FooList;


    //  Define a structure that will be on the list.

    //   (NOTE: To make debugging easier, it's best to define the LIST_ENTRY field
    //    as the very first field in your structure, but it's not a requirement.)

    typedef struct _FOO
    {
        LIST_ENTRY  FooListEntry;
        .
        .
        .
    }
    FOO, *PFOO;


    //  Initialize an empty list.

    InitializeListHead(&FooList);


    //  Create an object, append it to the end of the list.

    FOO*  pFoo;

    pFoo = ALLOC(sizeof(FOO));
    {check for errors, initialize FOO structure}

    InsertListTail(&FooList,&pFoo->FooListEntry);


    //  Scan list and delete selected items.

    LIST_ENTRY*  pListEntry = FooList.Flink;

    while (pListEntry != &FooList)
    {
        pFoo = CONTAINING_RECORD(pListEntry,FOO,FooListEntry);
        pListEntry = pListEntry->Flink;

        if (SomeFunction(pFoo))
        {
            RemoveListEntry(&pFoo->FooListEntry);
            FREE(pFoo);
        }
    }


    //  Purge all items from a list.

    while (!IsListEmpty(&FooList))
    {
        pListEntry = RemoveListHead(&FooList);
        pFoo = CONTAINING_RECORD(pListEntry,FOO,FooListEntry);
        FREE(pFoo);
    }
*/
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined( WIN32  ) || \
   ( defined( _MSVC_ ) && !defined(_WINNT_) && !defined(_WINNT_H) )

typedef struct _LIST_ENTRY
{
    struct  _LIST_ENTRY*  Flink;    // ptr to next link in chain
    struct  _LIST_ENTRY*  Blink;    // ptr to previous link in chain
}
LIST_ENTRY, *PLIST_ENTRY;

#endif  // !defined(_WINNT_)

//////////////////////////////////////////////////////////////////////////////////////////
//
// <typename>*  CONTAINING_RECORD
// (
//     VOID*        address,
//     <typename>   type,
//     <fieldname>  field
// );
//
/*
    Retrieves a typed pointer to a linked list item given the address of the
    link storage structure embedded in the linked list item, the type of the
    linked list item, and the field name of the embedded link storage structure.

    NOTE: since this macro uses compile-time type knowledge,
    there is no equivalent C procedure for this macro.

Arguments:

    address  -  The address of a LIST_ENTRY structure embedded in an a linked list item.
    type     -  The type name of the containing linked list item structure.
    field    -  The field name of the LIST_ENTRY structure embedded within the linked list item structure.

Return Value:

    Pointer to the linked list item.


For Example:

    If your record looked like this:

        typedef struct _MYRECORD
        {
            int         alpha;
            int         beta;
            LIST_ENTRY  gamma;
            int         delta;
            int         epsilon;
        }
        MYRECORD, *PMYRECORD;

    Then, given a variable called "pListEntry" that pointed to the LIST_ENTRY field
    within your record (i.e. gamma), you can obtain a pointer to the beginning of your
    record by coding the following CONTAINING_RECORD macro expression:

        MYRECORD*    pMyRecord;     // the variable you wish to point to your record
    //  LIST_ENTRY*  pListEntry;    // already points to the LIST_ENTRY field within
                                    // your record (i.e. points to field "gamma")

        pMyRecord = CONTAINING_RECORD(pListEntry,MYRECORD,gamma);
--*/

#ifndef  CONTAINING_RECORD
#define  CONTAINING_RECORD( address, type, field )                 \
                                                                   \
    ( (type*) ((char*)(address) - (char*)(&((type*)0)->field)) )
#endif


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//
//  From NTRTL.H:  Doubly-linked list manipulation routines.
//
//      (NOTE: implemented as macros but logically these are procedures)
//
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////
//
//  void  InitializeListHead
//  (
//      LIST_ENTRY*  head
//  );
//
/*
    Initializes a LIST_ENTRY structure to be the head of an initially empty linked list.

Arguments:

    head  -  Reference to the structure to be initialized.

Return Value:

    None
*/

#define InitializeListHead(head)                \
                                                \
    ( (head)->Flink = (head)->Blink = (head) )


//////////////////////////////////////////////////////////////////////////////////////////
// (I created this one myself -- Fish)
//
//  void  InitializeListLink
//  (
//      LIST_ENTRY*  link
//  );
//
/*
    Initializes a LIST_ENTRY structure
    to be an unlinked link.

Arguments:

    link  -  Reference to the structure to be initialized.

Return Value:

    None
*/

#define InitializeListLink(link)                \
                                                \
    ( (link)->Flink = (link)->Blink = (NULL) )


//////////////////////////////////////////////////////////////////////////////////////////
//
//  <boolean>  IsListEmpty
//  (
//      LIST_ENTRY*  head
//  );
//
/*
    Determines whether or not a list is empty.

Arguments:

    head  -  Reference to the head of the linked list to be examined.

Return Value:

    <true>   -  List is empty.
    <false>  -  List contains at least one item.
--*/

#define IsListEmpty(head)        \
                                 \
    ( (head)->Flink == (head) )


//////////////////////////////////////////////////////////////////////////////////////////
//
//  VOID  InsertListHead
//  (
//      LIST_ENTRY*  head,
//      LIST_ENTRY*  entry
//  );
//
/*
    Inserts a new item as the "head" (first) item of a linked list.

Arguments:

    head   -  Reference to the head of the linked list to be operated upon.
    entry  -  Reference to the linkage structure embedded in the linked list item
              to be added to the linked list.

Return Value:

    None
*/

#define InsertListHead(head,entry) \
{                                  \
    LIST_ENTRY*  _EX_Head;         \
    LIST_ENTRY*  _EX_Next;         \
                                   \
    _EX_Head  = (head);            \
    _EX_Next = _EX_Head->Flink;    \
                                   \
    (entry)->Flink = _EX_Next;     \
    (entry)->Blink = _EX_Head;     \
                                   \
    _EX_Head->Flink = (entry);     \
    _EX_Next->Blink = (entry);     \
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//  VOID  InsertListTail
//  (
//      LIST_ENTRY*  head,
//      LIST_ENTRY*  entry
//  );
//
/*
    Inserts a new item as the "tail" (last) item of a linked list.

Arguments:

    head   -  Reference to the head of the linked list to be operated upon.
    entry  -  Reference to the linkage structure embedded in the linked list item
              to be added to the linked list.

Return Value:

    None
*/

#define InsertListTail(head,entry) \
{                                  \
    LIST_ENTRY*  _EX_Head;         \
    LIST_ENTRY*  _EX_Tail;         \
                                   \
    _EX_Head  = (head);            \
    _EX_Tail = _EX_Head->Blink;    \
                                   \
    (entry)->Flink = _EX_Head;     \
    (entry)->Blink = _EX_Tail;     \
                                   \
    _EX_Tail->Flink = (entry);     \
    _EX_Head->Blink = (entry);     \
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//  LIST_ENTRY*  RemoveListHead
//  (
//      LIST_ENTRY*  head
//  );
//
/*
    Removes the "head" (first) item from a linked list, returning the pointer
    to the removed entry's embedded linkage structure. Attempting to remove the
    head item from a (properly initialized) linked list is a no-op and returns
    the pointer to the head of the linked list.

    The caller may use the CONTAINING_RECORD macro to amplify the returned
    linkage structure pointer to the containing linked list item structure.

Arguments:

    head  -  Reference to the head of the linked list to be operated upon.

Return Value:

    Returns a pointer to the newly removed linked list item's embedded linkage structure,
    or the linked list head in the case of an empty list.
*/

#define RemoveListHead(head)       \
                                   \
(head)->Flink;                     \
                                   \
{                                  \
    RemoveListEntry((head)->Flink) \
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//  LIST_ENTRY*  RemoveListTail
//  (
//      LIST_ENTRY*  head
//  );
//
/*
    Removes the "tail" (last) item from a linked list, returning the pointer to the
    removed entry's embedded linkage structure. Attempting to remove the tail item
    from a (properly initialized) linked list is a no-op and returns the pointer to
    the head of the linked list.

    The caller may use the CONTAINING_RECORD macro to amplify the returned
    linkage structure pointer to the containing linked list item structure.

Arguments:

    head  -  Reference to the head of the linked list to be operated upon.

Return Value:

    Pointer to the newly removed linked list item's embedded linkage structure,
    or the linked list head in the case of an empty list.
*/

#define RemoveListTail(head)       \
                                   \
(head)->Blink;                     \
                                   \
{                                  \
    RemoveListEntry((head)->Blink) \
}


//////////////////////////////////////////////////////////////////////////////////////////
//
//  VOID  RemoveListEntry
//  (
//      LIST_ENTRY*  entry
//  );
//
/*
    Removes an item from a linked list. (Removing the head of an empty list is a no-op.)

Arguments:

    entry  -  Reference to the linkage structure embedded in a linked list item structure.

Return Value:

    None
*/

#define RemoveListEntry(entry)    \
{                                 \
    LIST_ENTRY*  _EX_Blink;       \
    LIST_ENTRY*  _EX_Flink;       \
                                  \
    _EX_Flink = (entry)->Flink;   \
    _EX_Blink = (entry)->Blink;   \
                                  \
    _EX_Blink->Flink = _EX_Flink; \
    _EX_Flink->Blink = _EX_Blink; \
}

//////////////////////////////////////////////////////////////////////////////////////////

#endif // _LLIST_
