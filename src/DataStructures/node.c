//
// Created by paul magos on 07/01/22.
//
#include "../../headers/node.h"
#include <stdio.h>
#include <string.h>

Node createNode(char* index, void* data){
    Node tmp = (Node)malloc(sizeof(node));
    if(!tmp){
        // Failed to alloc
        perror("malloc");
        return NULL;
    }

    tmp->index = index;
    tmp->data = data;
    tmp->next=NULL;
    tmp->prev=NULL;

    return tmp;
}

void freeNode(Node* nodeToFree){
    if(nodeToFree){
        //if(nodeToFree->index) free(nodeToFree->index);
        if((*nodeToFree)->prev) (*nodeToFree)->prev->next = NULL;
        if((*nodeToFree)->next) (*nodeToFree)->next->prev = NULL;
        //if(nodeToFree->data) free(nodeToFree->data);
        free(*nodeToFree);
    }
}