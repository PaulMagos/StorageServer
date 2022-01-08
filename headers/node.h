//
// Created by paul magos on 07/01/22.
//

#ifndef STORAGESERVER_NODE_H
#define STORAGESERVER_NODE_H

#endif //STORAGESERVER_NODE_H

#include <stdlib.h>

typedef struct element{
    const char* key;
    void* data;
    struct element* next;
    struct element* prev;
} node;
/*
 * Double linked list's node with a key for implementation of hashmaps.
 * */

typedef node* Node;
// Definition of Node, so we don't have to write "node*" everytime


Node createNode(const char* key, void* data);
/*
 * Node allocation method
 * */

void freeNode(Node nodeToFree);