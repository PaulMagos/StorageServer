//
// Created by paul magos on 07/01/22.
//

#ifndef STORAGESERVER_NODE_H
#define STORAGESERVER_NODE_H

#endif //STORAGESERVER_NODE_H

#include <stdlib.h>

typedef struct element{
    char* index;
    char* data;
    struct element* next;
    struct element* prev;
} node;
/*
 * Double linked list's node with a index for implementation of hashmaps.
 * */

typedef node* Node;
// Definition of Node, so we don't have to write "node*" everytime


node* createNode(char* index, char* data);
/*
 * Node allocation method
 * */

void freeNode(node* nodeToFree);