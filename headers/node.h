//
// Created by paul magos on 07/01/22.
//

#ifndef STORAGESERVER_NODE_H
#define STORAGESERVER_NODE_H

#endif //STORAGESERVER_NODE_H

#include "utils.h"

typedef struct element{
    char* index;
    void* data;
    struct element* next;
    struct element* prev;
} node;
/*
 * Double linked list's node with a index for implementation of hashmaps.
 * */

typedef node* Node;
// Definition of Node, so we don't have to write "node*" everytime


Node createNode(char* index, void* data);
/*
 * Node allocation method
 * */

void freeNode(Node* nodeToFree);