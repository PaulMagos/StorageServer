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

List returnNewList();

int createList(List* myList);

void printList(Node head);

int pushTop(List* myList, char* index, void* data);

int pushBottom(List* myList, char* index, void* data);

int pullTop(List* myList, char** index, void** data);

int pullBottom(List* myList, char** index, void** data);

int deleteList(List* myList);

int search(Node head, char* str);

int searchInt(Node head, int num);

void getArg(Node head, char* str, char** dir);
