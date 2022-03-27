/*
 Created by paul on 11/01/22.
*/

#include "../../headers/client.h"
List commandList;
void freeList();
int main(int argc,char* argv[]){
    int del;

    /* Command list creation */
    del = createList(&commandList);
    if(del==-1){
        fprintf(stderr, "Error during list creation, errno = %d\n",errno);
        exit(errno);
    }
    atexit(freeList);
    /* Command list parsing */
    del = getCmdList(&commandList, argc, argv);
    if(del==-1){
        fprintf(stderr, "Error during list parsing, errno = %d\n", errno);
        exit(errno);
    }
    del = 0;
    return del;
}

void freeList(){
    int del;
    del = deleteList(&commandList);
    if(del==-1){
        fprintf(stderr, "Errore during list free, errno = %d\n", errno);
        exit(errno);
    }
}
