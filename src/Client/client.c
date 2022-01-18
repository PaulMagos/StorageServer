//
// Created by paul on 11/01/22.
//

#include <stdio.h>
#include "../../headers/utils.h"
#include "../../headers/commandParser.h"



int main(int argc,char* argv[]){
    int del=0;

    // Command list creation
    List commandList;
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

    return 0;
}

//void recIter(char* dirname, int cnt,  )