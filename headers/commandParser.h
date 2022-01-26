//
// Created by paul on 18/01/22.
//

#ifndef STORAGESERVER_COMMANDPARSER_H
#define STORAGESERVER_COMMANDPARSER_H

#include "api.h"
#include "utils.h"
#include "list.h"

int getCmdList(List* opList, int argc, char* argv[]);
void commandHandler(List* commandList);


#endif //STORAGESERVER_COMMANDPARSER_H
