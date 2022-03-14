//
// Created by paul magos on 07/01/22.
//
#include "../../headers/list.h"

List returnNewList(){
    List myList =  malloc (sizeof (list));
    if(!(myList)) {
        // ERRORE DOVUTO ALL'ALLOCAZIONE NON ANDATA A BUON FINE
        errno = ENOMEM;
        return NULL;
    }
    return myList;
}


int createList(List* myList){
    *myList =  calloc (1, sizeof (list));
    if(!(*myList)) {
        // ERRORE DOVUTO ALL'ALLOCAZIONE NON ANDATA A BUON FINE
        errno = ENOMEM;
        return -1;
    }
    return 0;
}

void printList(Node head){
    if(head == NULL) return;
    fprintf(stderr, "%s, %s\n", head->index, (char*)head->data);
    printList(head->next);
}

int pushTop(List* myList, char* index, void* data){
    if(!(*myList)){
        // ERRORE DOVUTO AD ARGOMENTO INVALIDO
        errno = EINVAL;
        return -1;
    }

    node* temp = createNode(index, data);
    if(!temp) {
        // ERRORE DOVUTO ALL'ALLOCAZIONE NON ANDATA A BUON FINE
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

int pushBottom(List* myList, char* index, void* data){
    if(!(*myList)){
        // ERRORE DOVUTO AD ARGOMENTO INVALIDO
        errno = EINVAL;
        return -1;
    }

    node* temp = createNode(index, data);
    if(!temp) {
        // ERRORE DOVUTO ALL'ALLOCAZIONE NON ANDATA A BUON FINE
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

int pullTop(List* myList, char** index, void** data){
    if(!(*myList) || (*myList)->len < 1){
        // ERRORE DOVUTO AD ARGOMENTO INVALIDO
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

    freeNode(&tmp);
    return 0;
}

Node getHead(List* myList){
    if(!(*myList) || (*myList)->len < 1){
        // ERRORE DOVUTO AD ARGOMENTO INVALIDO
        errno = EINVAL;
        return NULL;
    }

    node* tmp = (*myList)->head;

    (*myList)->len--;
    if((*myList)->len == 0){
        (*myList)->head = NULL;
        (*myList)->tail = NULL;
    }else {
        (*myList)->head = (*myList)->head->next;
    }

    return tmp;
}

int pullBottom(List* myList, char** index, void** data){
    if(!(*myList) || (*myList)->len < 1){
        // ERRORE DOVUTO AD ARGOMENTO INVALIDO
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

    freeNode(&tmp);
    return 0;
}

void freeNodes(Node* head){
    if((*head) == NULL) return;
    freeNodes(&(*head)->next);
    freeNode(&(*head));
    return;
}

int deleteList(List* myList) {
    if (!(*myList)) {
        // ERRORE DOVUTO AD ARGOMENTO INVALIDO
        errno = EINVAL;
        return -1;
    }

    freeNodes(&(*myList)->head);
    free(*myList);
    return 0;
}

int search(Node head, char* str){
    if(head == NULL) return 0;
    if (*head->index == *str) return 1;
    return search(head->next, str);
}

int searchInt(Node head, int num){
    if(head == NULL) return 0;
    if (*(int*)(head->data) == num) return 1;
    return searchInt(head->next, num);
}

void getArg(Node head, char* str, char** dir){
    if(head == NULL) return;
    if (*head->index == *str) *dir = head->data;
    getArg(head->next, str, &(*dir));
}

int pullByData(List* myList, void* data, Type_ type){
    if((*myList)->head == NULL) return -1;
    Node curr, prev;
    curr = (*myList)->head;
    switch (type) {
        case type_int:{
            while (*(int*)curr->next->data != *(int*)data || curr->next == NULL){
                prev = curr;
                curr = curr->next;
            }
            if(curr == (*myList)->head || curr == (*myList)->tail){
                if((*myList)->len-1 == 0) {
                    (*myList)->head = NULL;
                    (*myList)->tail = NULL;
                } else if(curr == (*myList)->head){
                    (*myList)->head = (*myList)->head->next;
                } else{
                    (*myList)->tail = (*myList)->tail->prev;
                }
            }
            if(curr->next == (*myList)->tail){
                prev->next = (*myList)->tail;
                (*myList)->tail->prev = prev;
            } else if(curr->prev == (*myList)->head){
                (*myList)->head->next = curr->next;
                curr->next->prev = (*myList)->head;
            } else {
                prev->next = curr->next;
                curr->next->prev = prev;
            }
            (*myList)->len--;
            return 0;
        }
        case type_char:{
            fprintf(stderr, "ERROR - Not Implemented, exiting...");
            exit(0);
        }
        case type_string:{
            fprintf(stderr, "ERROR - Not Implemented, exiting...");
            exit(0);
        }
    }
    return -1;
}