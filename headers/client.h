//
// Created by paul on 26/01/22.
//

#ifndef STORAGESERVER_CLIENT_H
#define STORAGESERVER_CLIENT_H

#include "list.h"
#include "api.h"

int getCmdList(List* opList, int argc, char* argv[]);
void commandHandler(List* commandList);

#endif //STORAGESERVER_CLIENT_H
