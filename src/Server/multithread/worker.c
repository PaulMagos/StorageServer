//
// Created by paul on 26/01/22.
//

#include "../../../headers/worker.h"

typedef struct {
    int pipe;
    int client_fd;
    int worker_id;
} wTask;

static int CloseConnection(int fd_client, int workerid);

void taskExecute(void* argument){
    if( argument == NULL ) {
        fprintf(stderr, "ERROR - Invalid Worker Argument, errno = %d", EINVAL);
        exit(EXIT_FAILURE);
    }

    wTask *tmpArgs = argument;
    int myId = tmpArgs->worker_id;
    int fd_client = tmpArgs->client_fd;
    int pipe = tmpArgs->pipe;
    free(argument);

    operation operation1;

    int readed;
    if((readed = readn(fd_client, &operation1, sizeof(operation))) == -1){
        perror("readn");
        if(writen(fd_client, &errno, sizeof(int)) == -1){
            perror("writen");
        }
        return;
    }

    if(readed == 0){
        // client non più connesso
        if(CloseConnection(fd_client, myId) == -1) perror("CloseConnection");
        return;
    } else{
        switch (operation1) {
            case of:{
                break;
            }   // openFile
            case wr:{
                break;
            }   // write
            case r:{
                break;
            }   // read file
            case rn:{
                break;
            }   // read nFiles
            case lk:{
                break;
            }   // lock file
            case unlk:{
                break;
            }   // unlock file
            case del:{
                break;
            }   // delete file
            case cl:{
                break;
            }   // close file
            case ap:{
                break;
            }   // append to file
        }
    }
}

static int CloseConnection(int fd_client, int workerid){
    if(fd_client == -1 || workerid == -1){
        errno = EINVAL;
        return -1;
    }
    SYSCALL_RETURN(closeConn_pthread_mutex_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "Error - Server Lock failure, errno = %d", errno);

    fileServer file;
    icl_hash_t *currentbucket, *current;

    int i = 0;

}