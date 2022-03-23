/*
 * Created by paul magos on 07/01/22.
 */
#include "../../headers/node.h"
#include <string.h>
Node createNode(char* index, void* data){
    Node tmp = (Node)malloc(sizeof(node));
    size_t  argSize;
    char* argument;
    if(!tmp){
        /* Failed to alloc */
        perror("malloc");
        return NULL;
    }
    if(index != NULL) {
        tmp->index = malloc(strlen(index)+1);
        strncpy(tmp->index, index, strlen(index)+1);
        argument = (char*)data;
        if(data!=NULL){
            argSize = strlen(argument)+1;
            tmp->arg.data = malloc(argSize);
            memcpy(tmp->arg.data, argument, argSize);
        } else tmp->arg.data = NULL;
    } else{
        tmp->index = NULL;
        tmp->arg.dataInt = *(int*)data;
    }
    tmp->next=NULL;
    tmp->prev=NULL;

    return tmp;
}

void freeNode(Node nodeToFree){
    if(nodeToFree){
        if((nodeToFree)->index) {
            if((nodeToFree)->arg.data!= NULL) free((nodeToFree)->arg.data);
            free((nodeToFree)->index);
        }
        free(nodeToFree);
    }
}