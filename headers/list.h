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

int pushTop(List myList, const char* key, void* data);

int pushBottom(List myList, const char* key, void* data);

int pullTop(List myList, const char* key, void* data);

int pullBottom(List myList, const char* key, void* data);
