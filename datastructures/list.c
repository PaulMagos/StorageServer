//
// Created by paul magos on 07/01/22.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../headers/utils.h"
#include "../headers/list.h"

int createList(List* myList){
    *myList = (List) malloc (sizeof (list));
    if(!(*myList)) {
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

void printList(Node head){
    if(head == NULL) return;
    fprintf(stderr, "%s, %s\n", head->index, head->data);
    printList(head->next);
}

int pushTop(List* myList, char* index, char* data){
    if(!(*myList)){
        // ERRORE *******************************
        errno = EINVAL;
        return -1;
    }

    node* temp = createNode(index, data);
    if(!temp) {
        errno = ENOMEM;
        return -1;
    }

    if((*myList)->len>0){
        temp->next = (*myList)->head;
        (*myList)->head->prev = temp;
    } else (*myList)->tail = temp;

    (*myList)->head = temp;
    (*myList)->len++;
    return 0;
}

int pushBottom(List* myList, char* index, char* data){
    if(!(*myList)){
        // ERRORE *******************************
        errno = EINVAL;
        return -1;
    }

    node* temp = createNode(index, data);
    if(!temp) {
        errno = ENOMEM;
        return -1;
    }

    if((*myList)->len>0){
        temp->prev = (*myList)->tail;
        (*myList)->tail->next = temp;
    } else (*myList)->head = temp;

    (*myList)->tail = temp;
    (*myList)->len++;
    return 0;
}

int pullTop(List* myList, char** index, char** data){
    if(!(*myList) || (*myList)->len < 1){
        // ERRORE DA IMPLEMENTARE *******************************
        errno = EINVAL;
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
        errno = EINVAL;
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

void freeNodes(Node head){
    if(head == NULL) return;
    freeNodes(head->next);
    freeNode(head);
    return;
}

int deleteList(List* myList) {
    if (!(*myList)) {
        // No list
        errno = EINVAL;
        return -1;
    }

    freeNodes((*myList)->head);
    free(*myList);
    return 0;
}

bool search(Node head, char* str){
    if(head == NULL) return false;
    if (strcmp(head->index, str) == 0) return true;
    if(head->next) search(head->next, str);
    return false;
}
