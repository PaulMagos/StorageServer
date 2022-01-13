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

typedef list* List;

List createList();

void printList(List myList);

int pushTop(List* myList, char* index, char* data);

int pushBottom(List* myList, char* index, char* data);

int pullTop(List* myList, char** index, char** data);

int pullBottom(List* myList, char** index, char** data);

int deleteList(List* myList);
