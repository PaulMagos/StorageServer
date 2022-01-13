//
// Created by paul magos on 07/01/22.
//

#include <stdio.h>
#include "../headers/list.h"

List createList(){
    List myList = (List) malloc(sizeof (list));
    if(!myList){
        // Failed to alloc
        perror("malloc");
        return NULL;
    }

    myList->head = NULL;
    myList->tail = NULL;
    myList->len = 0;
    return myList;
}


void recPrint(Node head){
    if(head == NULL) return;
    fprintf(stderr, "%s %s \n", head->index, (char*)head->data);
    recPrint(head->next);
}

void printList(List myList){
    if(myList == NULL || myList->head == NULL) {
        return;
    }else {
        recPrint(myList->head);
    }
}

int pushTop(List* myList, char* index, char* data){

    if(!(*myList)){
        // ERRORE DA IMPLEMENTARE *******************************
        perror("list null");
        return -1;
    }

    node* tmp = createNode(index, data);
    if(!tmp) return -1;

    if((*myList)->len>0){
        tmp->next = (*myList)->head;
        (*myList)->head->prev = tmp;
    } else (*myList)->tail = tmp;

    (*myList)->head = tmp;
    (*myList)->len++;
    return 0;
}

int pushBottom(List* myList, char* index, char* data){
    if(!(*myList)){
        // ERRORE DA IMPLEMENTARE *******************************
        return -1;
    }

    node* tmp = createNode(index, data);
    if(!tmp) return -1;

    if((*myList)->len>0){
        tmp->prev = (*myList)->tail;
        (*myList)->tail->next = tmp;
    } else (*myList)->head = tmp;

    (*myList)->tail = tmp;
    (*myList)->len++;
    return 0;
}

int pullTop(List* myList, char** index, char** data){
    if(!(*myList) || (*myList)->len < 1){
        // ERRORE DA IMPLEMENTARE *******************************
        return -1;
    }

    node* tmp = (*myList)->head;

    (*myList)->len--;
    if((*myList)->len == 0){
        (*myList)->head = NULL;
        (*myList)->tail = NULL;
    }else {
        (*myList)->head = (*myList)->head->next;
    }

    *index = tmp->index;
    *data = tmp->data;

    freeNode(tmp);
    return 0;
}

int pullBottom(List* myList, char** index, char** data){
    if(!(*myList) || (*myList)->len < 1){
        // ERRORE DA IMPLEMENTARE *******************************
        return -1;
    }

    node* tmp = (*myList)->tail;

    (*myList)->len--;
    if((*myList)->len == 0){
        (*myList)->head = NULL;
        (*myList)->tail = NULL;
    }else {
        (*myList)->tail = (*myList)->tail->prev;
    }

    *index = tmp->index;
    *data = tmp->data;

    freeNode(tmp);
    return 0;
}

int deleteList(List myList){

    printf("ciao");
    if(!(myList)){
        // No list
        // ERRORE DA IMPLEMENTARE *******************************
        return -1;
    }

    node* p = (myList)->head;


    while(p!=NULL && (myList)->head!=(myList)->tail) {
        p = (myList)->head->next;
        freeNode((myList)->head);
    };

    free(myList);
    return 0;
}