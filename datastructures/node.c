//
// Created by paul magos on 07/01/22.
//
#include "../headers/node.h"
#include <errno.h>
#include <stdio.h>

node* createNode(char* index, char* data){
    node* tmp = (node*)malloc(sizeof(node));
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

void freeNode(node* nodeToFree){
    if(nodeToFree){
        if(nodeToFree->index) free(nodeToFree->index);
        if(nodeToFree->data) free(nodeToFree->data);
        free(nodeToFree);
    }
}