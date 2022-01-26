//
// Created by paul on 11/01/22.
//

#include "../../headers/client.h"

int main(int argc,char* argv[]){
    int del;

    // Command list creation
    List commandList = NULL;
    SYSCALL_EXIT(createList,
                 del,
                 createList(&commandList),
                 "Error during list creation, errno = %d\n",
                 errno);
    // Command list parsing
    SYSCALL_EXIT(getCmdList,
                 del,
                 getCmdList(&commandList, argc, argv),
                 "Error during list parsing, errno = %d\n",
                 errno);
    // Free Command List
    SYSCALL_EXIT(deleteList,
                 del,
                 deleteList(&commandList),
                 "Errore during list free, errno = %d\n",
                 errno);
    del = 0;
    return del;
}