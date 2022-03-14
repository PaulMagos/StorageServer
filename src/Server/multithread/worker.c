//
// Created by paul on 26/01/22.
//

#include "../../../headers/server.h"


int opExecute(int fd, int workerId, operation op);

void taskExecute(void* argument){
    if( argument == NULL ) {
        fprintf(stderr, "ERROR - Invalid Worker Argument, errno = %d", EINVAL);
        exit(EXIT_FAILURE);
    }

    wTask *tmpArgs = argument;
    int myId = tmpArgs->worker_id;
    int fd_client = (int)tmpArgs->client_fd;
    //int pipe = tmpArgs->pipeT;
    free(argument);

    appendOnLog(ServerLog, "\t[Thread %d]:\t Client %d serving\n", myId, fd_client);
    operation operation1;

    int readed = readOp(fd_client, &operation1);
    // If -1 error in communication
    if(readed == -1) return;

    // If 0 fd disconnected
    if(readed == 0){
        // client non più connesso
        if(CloseConnection((int)fd_client, myId) == -1)
            fprintf(stderr, "ERROR - Closing connection to client fault, errno = %d", errno);
        return;
    } else{
        if(opExecute(fd_client, myId, operation1) == 1) {
            int success = 0;
            if(writen(fd_client, &success, sizeof(int)) == -1){
                fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
            }
        }
    }

}

int opExecute(int fd, int workerId, operation op){
    int res;
    switch (op) {
        case of:{
            res = OpenFile(fd, (int)workerId);
            break;
        }   // openFile
        case wr:{
            res = ReceiveFile(fd, (int)workerId);
            break;
        }   // write
        case r:{
            res = SendFile(fd, (int)workerId);
            break;
        }   // read file
        case rn:{
            res = SendNFiles(fd, (int)workerId);
            break;
        }   // read nFiles
        case lk:{
            res = LockFile(fd, (int)workerId);
            break;
        }   // lock file
        case unlk:{
            res = UnLockFile(fd, (int)workerId);
            break;
        }   // unlock file
        case del:{
            res = DeleteFile(fd, (int)workerId);
            break;
        }   // delete file
        case cl:{
            res = CloseFile(fd, (int)workerId);
            break;
        }   // close file
        case ap:{
            res = AppendOnFile(fd, (int)workerId);
            break;
        }   // append to file
    }

    if(res == -1){
        // File open error, send error to client
        if(writen((int)fd, &errno, sizeof(int )) == -1){
            fprintf(stderr, "ERROR - Sending info to client fault, errno = %d", errno);
        }
        if(errno == EBADMSG){
            if(CloseConnection((int)fd, (int)workerId) == -1)
                fprintf(stderr, "ERROR - Closing connection to client fault, errno = %d", errno);
        }
        return -1;
    }

    switch (op) {
        case wr:
        case r:
        case rn:
        case ap: break;
        default:{
            return 1;
        }
    }
    return 0;
}


int CloseConnection(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1){
        errno = EINVAL;
        return -1;
    }
    SYSCALL_RETURN(closeConn_pthread_mutex_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "Error - Server Lock failure, errno = %d", errno);

    serverFile* file;
    icl_entry_t *bucket, *curr;

    for(int i = 0; i < ServerStorage->filesTable->nbuckets; i++){
        bucket = ServerStorage->filesTable->buckets[i];
        for(curr = bucket; curr != NULL; curr=curr->next){
            file = ((serverFile*)curr->data);
            if(file->client_fd==fd_client) {
                if(decrementLockFile(file, (file->writers!=0)? 1:0, fd_client) != 0) return -1;
            }
        }
    }


    SYSCALL_RETURN(closeConn_pthread_mutex_lock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "Error - Server unLock failure, errno = %d", errno);
    return 0;
}

int OpenFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }
    int len;
    void* buffer;
    readDataLength(fd_client, &len, &buffer);

    int flag;
    int readed = readn(fd_client, &flag, (sizeof(int)));
    if(readed == -1) {
        fprintf(stderr, "ERROR - Reading file open flags from %d, errno = %d", fd_client, errno);
        free(buffer);
        return -1;
    } else if(readed == 0){
        // ERROR IN COMMUNICATION
        free(buffer);
        errno = EBADMSG;
        return -1;
    }

    SYSCALL_RETURN(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno);

    serverFile* File = icl_hash_find(ServerStorage->filesTable, buffer);
    switch (flag) {
        case O_CREAT:{
            if(File == NULL){
                break;
            }
            break;
        }
        case 1:{
            break;
        }
    }


    return 0;
}


int incrementLockFile(serverFile* file, int mode, int locker){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->order), "ERROR - Mutex lock of file failed, %s\n", file->path);
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    while (file->writers!=0 || ((mode==0)? 1 : file->readers>0)){
        SYSCALL_RETURN(cond_wait, pthread_cond_wait(&(file->condition), &(file->lock)) ,
                       "ERROR - Condition wait for file %s failed\n", file->path);
    }
    file->readers++;
    if(locker!=-1) file->client_fd = locker;
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->order)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return 0;
}

int decrementLockFile(serverFile* file, int mode, int locker){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    if((locker!=-1)? file->client_fd == locker : 1){
        if(mode == 0) {
        file->readers--;
        if(file->readers == 0) SYSCALL_RETURN(cond_broad, pthread_cond_broadcast(&(file->condition)),
                                              "ERROR - Condition broadcast reader release file %s", file->path);
        }
        else {
            file->writers--;
            SYSCALL_RETURN(cond_broad, pthread_cond_broadcast(&(file->condition)),
                           "ERROR - Condition broadcast writer release file %s", file->path);
        }
    } else return -1;
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return 0;
}

wTask* taskCreate (int pipe, int workerId, int client_fd){
    if(client_fd == -1 || workerId != -1) return NULL;
    wTask *task;
    task = (wTask*)malloc(sizeof(wTask));
    task->pipeT = pipe;
    task->worker_id = workerId;
    task->client_fd = client_fd;
    return task;
}

int taskDestroy(wTask* task){
    if(task != NULL) {
        free(task);
        return 0;
    }
    return -1;
}


