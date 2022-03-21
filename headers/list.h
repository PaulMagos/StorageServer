//
// Created by paul magos on 07/01/22.
//

#ifndef STORAGESERVER_LIST_H
#define STORAGESERVER_LIST_H

#endif //STORAGESERVER_LIST_H

#include "node.h"

typedef struct el{
    Node head;
    Node tail;
    unsigned int len;
} list;

typedef enum {
    type_int,
    type_char,
    type_string,
} Type_;

typedef list* List;
/**
 *   @func returnNewList
 *   @effects -> allocates space
 *   @return -> returns NULL if there's a failure, an allocated List otherwise
 */
List returnNewList();
/**
 *   @func  createList
 *   @param myList -> variable List to allocate
 *   @effects -> allocates space
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int createList(List* myList);

/**
 *   @func  printList
 *   @param head -> List head
 *   @effects -> Prints the list from head till curr->next == NULL
 */
void printList(Node head);

/**
 *   @func  pushTop
 *   @param myList -> List
 *   @param index -> index of the node
 *   @param data -> data to insert
 *   @effects -> Pushes a new node with index and data to the top of the List myList
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int pushTop(List* myList, char* index, void* data);
/**
 *   @func  pushBottom
 *   @param myList -> List
 *   @param index -> index of the node
 *   @param data -> data to insert
 *   @effects -> Pushes a new node with index and data to the bottom of the List myList
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int pushBottom(List* myList, char* index, void* data);
/**
 *   @func  pullTop
 *   @param myList -> List
 *   @param index -> index for the node index
 *   @param data -> data for the node data
 *   @effects -> Pulls a node with index and data from the top of the List myList
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int pullTop(List* myList, char** index, void** data);

/**
 *   @func  removeByInt
 *   @param myList -> List
 *   @param data -> data int remove
 *   @effects -> Pulls a node with index and data from the the list and removes it
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int removeByInt(List* myList, void* data);
/**
 *   @func  pullBottom
 *   @param myList -> List
 *   @param index -> index for the node index
 *   @param data -> data for the node data
 *   @effects -> Pulls a node with index and data from the bottom of the List myList
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int pullBottom(List* myList, char** index, void** data);


/**
 *   @func  deleteList
 *   @param myList -> List to delete
 *   @effects -> deletes the list and the nodes in the list
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int deleteList(List* myList);


/**
 *   @func  search
 *   @param head -> Head of the list
 *   @param str -> String to compare with the index of the node
 *   @return -> returns 0 if there's not index in the list, 1 if the index is in the list
 */
int search(Node head, char* str);
/**
 *   @func  searchInt
 *   @param head -> Head of the list
 *   @param num -> Num to compare with the data in the node
 *   @return -> returns 0 if there's no num in the list, 1 if the num is in the list
 */
int searchInt(Node head, int num);
/**
 *   @func  getArg
 *   @param head -> Head of the list
 *   @param str -> index of the node
 *   @param dir -> string where to save the data
 */
void getArg(Node head, char* str, char** dir);
