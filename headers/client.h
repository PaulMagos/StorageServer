//
// Created by paul on 26/01/22.
//

#ifndef STORAGESERVER_CLIENT_H
#define STORAGESERVER_CLIENT_H

#include "list.h"
#include "api.h"
/**
 *   @func  getCmdList
 *   @param opList -> An empty list to insert the various commands
 *   @param argc -> num of operations
 *   @param argv -> va_args associated with the operations
 *   @effects -> parse all the commands from the terminal
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int getCmdList(List* opList, int argc, char* argv[]);
/**
 *   @func  commandHandler
 *   @param commandList -> the list of commands with arguments
 *   @effects -> sends all the requests to the server and handle the files
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
void commandHandler(List* commandList);

#endif //STORAGESERVER_CLIENT_H
