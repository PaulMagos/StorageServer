//
// Created by paul magos on 07/01/22.
//

#include "../headers/list.h"

List createList(){
    List list1 = (List) malloc(sizeof (list));
    if(!list1){
        // Failed to alloc
        // ERRORE DA IMPLEMENTARE *******************************
        return NULL;
    }

    list1->head = NULL;
    list1->tail = NULL;
    list1->len = 0;
    return list1;
}

int pushTop(List list1, const char* key, void* data){
    if(!list1){
        // ERRORE DA IMPLEMENTARE *******************************
        return -1;
    }

    Node tmp = createNode(key, data);
    if(!tmp) return -1;

    if(list1->len>0){
        tmp->next = list1->head;
        list1->head->prev = tmp;
    } else list1->tail = tmp;

    list1->head = tmp;
    list1->len++;

    return 0;
}

int pushBottom(List list1, const char* key, void* data){
    if(!list1){
        // ERRORE DA IMPLEMENTARE *******************************
        return -1;
    }

    Node tmp = createNode(key, data);
    if(!tmp) return -1;

    if()
}
