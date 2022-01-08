//
// Created by paul magos on 07/01/22.
//
#include "../headers/node.h"

Node createNode(const char* key, void* data){
    Node tmp = (Node)malloc(sizeof(node));
    if(!tmp){
        // Failed to alloc
        // ERRORE DA IMPLEMENTARE ***********************************
        return NULL;
    }

    tmp->key = key;
    tmp->data = data;
    tmp->next=tmp->prev=NULL;

    return tmp;
}

void freeNode(Node nodeToFree){
    if(nodeToFree){
        if(nodeToFree->key) free((void*)nodeToFree->key);
        if(nodeToFree->data) free(nodeToFree->data);
        free(nodeToFree);
    }
}