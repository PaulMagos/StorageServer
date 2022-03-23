//
// Created by paul magos on 07/01/22.
//
#include "../../headers/node.h"
#include <string.h>
Node createNode(char* index, void* data){
    Node tmp = (Node)malloc(sizeof(node));
    if(!tmp){
        // Failed to alloc
        perror("malloc");
        return NULL;
    }
    if(index != NULL) {
        tmp->index = malloc(strlen(index)+1);
        strncpy(tmp->index, index, strlen(index)+1);
        char* argument = (char*)data;
        if(data!=NULL){
            size_t  argSize = strlen(argument)+1;
            tmp->data = malloc(argSize);
            memcpy(tmp->data, argument, argSize);
        } else tmp->data = NULL;
    } else{
        tmp->index = NULL;
        tmp->dataInt = *(int*)data;
    }
    tmp->next=NULL;
    tmp->prev=NULL;

    return tmp;
}

void freeNode(Node nodeToFree){
    if(nodeToFree){
        if((nodeToFree)->index) {
            if((nodeToFree)->data!= NULL) free((nodeToFree)->data);
            free((nodeToFree)->index);
        }
        free(nodeToFree);
    }
}