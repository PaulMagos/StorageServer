//
// Created by paul on 26/01/22.
//

#ifndef STORAGESERVER_THREAD_H
#define STORAGESERVER_THREAD_H

#include "server.h"
#include "icl_hash.h"

typedef struct {
    int pipe;
    int client_fd;
    int worker_id;
} wTask;

void taskExecute (void* argument);

#endif //STORAGESERVER_THREAD_H
