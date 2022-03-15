//
// Created by paul on 26/01/22.
//

#include "../../../headers/server.h"


int opExecute(int fd, int workerId, operation op);
int ExpelledHandler(int fd, int workerId, List expelled);

void taskExecute(void* argument){
    if( argument == NULL ) {
        fprintf(stderr, "ERROR - Invalid Worker Argument, errno = %d", EINVAL);
        exit(EXIT_FAILURE);
    }

    icl_hash_insert(ServerStorage->filesTable, "ciao", "ciao");
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

    int* tmp = &fd_client;
    for(int i = 0; i < ServerStorage->filesTable->nbuckets; i++){
        bucket = ServerStorage->filesTable->buckets[i];
        for(curr = bucket; curr != NULL; curr=curr->next){
            file = ((serverFile*)curr->data);
            if(file->writers==0) {
                fileReadersDecrement(file);
                fileWritersIncrement(file);
            }
            if(searchInt(file->client_fd->head, fd_client) == 0){
                pullByData(&(file->client_fd), (void*)tmp, type_int);
                file->lockFd = -1;
                fileWritersDecrement(file);
            }
        }
    }
    SYSCALL_RETURN(closeConn_pthread_mutex_lock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "Error - Server unLock failure, errno = %d", errno);

    // Removing client from connected clients
    if(clientRemove(fd_clients, fd_client) != 0) return -1;
    return 0;
}

int OpenFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    readDataLength(fd_client, &length, &buffer);

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
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(ServerStorage->actualFilesNumber==ServerConfig.maxFile){
        free(buffer);
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock release failure, errno = %d", errno)
        return -1;
    }
    serverFile* File = icl_hash_find(ServerStorage->filesTable, buffer);

    int* tmp = &fd_client;
    switch (flag) {
        case O_CREAT:{
            if(File!=NULL) {
                free(buffer);
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = EEXIST;
                return -1;
            }
            else{
                File = malloc(sizeof(serverFile));
                if(!File) {
                    free(buffer);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->creationTime))==-1){
                    free(buffer);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->lastOpTime))==-1){
                    free(buffer);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                File->path = malloc(length);
                if(!(File->path)){
                    free(buffer);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(createList(&(File->client_fd))!=0){
                    free(buffer);
                    free(File->path);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pushTop(&(File->client_fd), NULL, tmp)!=0){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_mutex_init(&(File->lock), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_mutex_init(&(File->order), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    pthread_mutex_destroy(&(File->lock));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_cond_init(&(File->condition), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    pthread_mutex_destroy(&(File->lock));
                    pthread_mutex_destroy(&(File->order));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(icl_hash_insert(ServerStorage->filesTable, buffer, File) == NULL){
                    free(buffer);
                    fprintf(stderr, "ERROR - Insert file %s failure", File->path);
                    freeFile(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                File->size = 0;
                File->writers = 0;
                File->readers = 0;
                File->lockFd = -1;
                File->toDelete = 0;
                File->content = NULL;
                File->latsOp = O_CREAT;
                fileNumIncrement();
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                appendOnLog(ServerLog, "[Thread %d]: File %s created for client %d", workerId, File->path, fd_client);
                free(buffer);
                return 0;
            }
            break;
        }
        case O_LOCK:{
            if(File!=NULL){
                if(isLocked(File, fd_client) == 0){
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    errno = EDEADLK;
                    return -1;
                }
                if(isLocked(File, fd_client) == 1){
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    errno = EBUSY;
                    return -1;
                }
                fileWritersIncrement(File);
                SYSCALL_RETURN(pthread_lock, pthread_mutex_lock(&(File->lock)),
                               "ERROR - Lock file %s failure", File->path)

                if(searchInt(File->client_fd->head, fd_client)==1){
                    free(buffer);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                pushTop(&(File->client_fd), NULL, tmp);
                File->lockFd = fd_client;
                SYSCALL_RETURN(pthread_unlock, pthread_mutex_unlock(&(File->lock)),
                               "ERROR - Unlock file %s failure", File->path)
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                fileWritersDecrement(File);
                appendOnLog(ServerLog,"[Thread %d]: File %s locked by client %d", workerId, File->path, fd_client);
                free(buffer);
                return 0;
            } else{
                free(buffer);
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = ENOENT;
                return -1;
            }
        }
        case (O_CREAT|O_LOCK):{
            if(File==NULL) {
                File = malloc(sizeof(serverFile));
                if(!File) {
                    free(buffer);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->creationTime))==-1){
                    free(buffer);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->lastOpTime))==-1){
                    free(buffer);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                File->path = malloc(length);
                if(!(File->path)){
                    free(buffer);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(createList(&(File->client_fd))!=0){
                    free(buffer);
                    free(File->path);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_mutex_init(&(File->lock), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_mutex_init(&(File->order), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    pthread_mutex_destroy(&(File->lock));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_cond_init(&(File->condition), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    deleteList(&(File->client_fd));
                    pthread_mutex_destroy(&(File->lock));
                    pthread_mutex_destroy(&(File->order));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(icl_hash_insert(ServerStorage->filesTable, buffer, File) == NULL){
                    free(buffer);
                    fprintf(stderr, "ERROR - Insert file %s failure", File->path);
                    freeFile(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                File->size = 0;
                File->writers = 0;
                File->readers = 0;
                File->lockFd = -1;
                File->toDelete = 0;
                File->content = NULL;
                File->latsOp = O_CREAT;
                fileNumIncrement();
                appendOnLog(ServerLog,"[Thread %d]: File %s created by client %d", workerId, File->path, fd_client);
            }
            if(isLocked(File, fd_client) == -1){
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = EDEADLK;
                return -1;
            }
            if(isLocked(File, fd_client) == 1){
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = EBUSY;
                return -1;
            }
            fileWritersIncrement(File);
            SYSCALL_RETURN(pthread_lock, pthread_mutex_lock(&(File->lock)),
                           "ERROR - Lock file %s failure", File->path)
            if(searchInt(File->client_fd->head, fd_client)!=1){
                if(pushTop(&(File->client_fd), NULL, tmp)!=0){
                    free(buffer);
                    freeFile(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
            }
            File->lockFd = fd_client;
            SYSCALL_RETURN(pthread_unlock, pthread_mutex_unlock(&(File->lock)),
                           "ERROR - Unlock file %s failure", File->path)
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock release failure, errno = %d", errno)
            fileWritersDecrement(File);
            appendOnLog(ServerLog,"[Thread %d]: File %s locked by client %d", workerId, File->path, fd_client);
            free(buffer);
            return 0;
        }
        default:{
            if(File==NULL){
                free(buffer);
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = ENOENT;
                return -1;
            }
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock release failure, errno = %d", errno)
            if(File->lockFd!=-1 && File->lockFd==fd_client){
                free(buffer);
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                return -1;
            }

            fileWritersIncrement(File);
            if(searchInt(File->client_fd->head, fd_client)!=1) {
                if(pushTop(&(File->client_fd), NULL, tmp)!=0){
                    free(buffer);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                appendOnLog(ServerLog, "[Thread %d]: File %s opened by client %d", workerId, File->path, fd_client);
            }
            fileWritersDecrement(File);
            free(buffer);
            return 0;
        }
    }
}

int CloseFile(int fd_client, int workerId);
int DeleteFile(int fd_client, int workerId);
int ReceiveFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    readDataLength(fd_client, &length, &buffer);

    SYSCALL_RETURN(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, buffer);
    if(File==NULL){
        free(buffer);
        errno = ENOENT;
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }
    free(buffer);
    fileWritersIncrement(File);

    if(File->content!=NULL || searchInt(File->client_fd->head, fd_client)==0 ||
    (File->lockFd!=-1 && File->lockFd!=fd_client)){
        // File already exists, use append to add new content
        fileWritersDecrement(File);
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }

    // File Ready, say it to Client
    int success = 0;
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
    }


    readDataLength(fd_client, &length, &buffer);

    if((size_t)length > ServerConfig.maxByte){
        errno = EFBIG;
        free(buffer);
        fileWritersDecrement(File);
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }

    List expelled;
    createList(&expelled);
    while (ServerStorage->actualFilesBytes+(size_t)length > ServerConfig.maxByte){
        serverFile* file = icl_hash_toReplace(ServerStorage->filesTable, ServerConfig.policy);
        if(file == NULL) {
            free(buffer);
            fileWritersDecrement(File);
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return -1;
        }
        pushTop(&expelled, file->path, NULL);
    }

    fileBytesIncrement((size_t)length);
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    File->content = malloc(sizeof((size_t)length));
    memcpy(File->content, buffer, (size_t)length);
    free(buffer);
    File->latsOp = O_WRITE;
    File->size = (size_t)length;
    fileWritersDecrement(File);
    // File Found, say it to Client
    int success = 0;
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
    }
    appendOnLog(ServerLog, "[Thread %d]: File %s received from client %d", workerId, File->path, fd_client);
    if(ExpelledHandler(fd_client, workerId, expelled)==-1){
        deleteList(&expelled);
        errno = EBADMSG;
        return -1;
    }
    return 0;
}
int SendFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    readDataLength(fd_client, &length, &buffer);
    SYSCALL_RETURN(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, buffer);
    if(File==NULL){
        free(buffer);
        errno = ENOENT;
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }
    free(buffer);
    fileReadersIncrement(File);
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    if(searchInt(File->client_fd->head, fd_client)==-1){
        errno = EACCES;
        fileReadersDecrement(File);
        return -1;
    }
    // File Found, say it to Client
    int success = 0;
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
    }

    int bytes = (int)File->size;
    buffer = File->content;
    writeDataLength(fd_client, &bytes, &buffer);
    fileReadersDecrement(File);

    appendOnLog(ServerLog, "[Thread %d]: File %s sent to client %d", workerId, File->path, fd_client);
    return 0;

}
int SendNFiles(int fd_client, int workerId);
int LockFile(int fd_client, int workerId);
int UnLockFile(int fd_client, int workerId);
int AppendOnFile(int fd_client, int workerId);


int ExpelledHandler(int fd, int workerId, List expelled){

    return 0;
}

int fileReadersIncrement(serverFile* file){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->order), "ERROR - Mutex lock of file failed, %s\n", file->path);
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    while (file->writers!=0) {
        SYSCALL_RETURN(cond_wait, pthread_cond_wait(&(file->condition), &(file->lock)),
                       "ERROR - Condition wait for file %s failed\n", file->path);
    }
    file->readers++;
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->order)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return 0;
}
int fileReadersDecrement(serverFile* file){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    file->readers--;
    if(file->readers == 0) {
        SYSCALL_RETURN(cond_broad, pthread_cond_broadcast(&(file->condition)),
                       "ERROR - Condition broadcast reader release file %s", file->path);
    }
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return 0;
}
int fileWritersIncrement(serverFile* file){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->order), "ERROR - Mutex lock of file failed, %s\n", file->path);
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    while (file->writers!=0 || file->readers!=0) {
        SYSCALL_RETURN(cond_wait, pthread_cond_wait(&(file->condition), &(file->lock)),
                       "ERROR - Condition wait for file %s failed\n", file->path);
    }
    file->writers++;
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->order)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return 0;
}
int fileWritersDecrement(serverFile* file){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    SYSCALL_RETURN(cond_broad, pthread_cond_broadcast(&(file->condition)),
                   "ERROR - Condition broadcast reader release file %s", file->path);
    file->writers--;
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return 0;
}
int isLocked(serverFile* file, long fd){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    if(file->lockFd == fd){
        SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                       "ERROR - Mutex unlock of file failed, %s\n", file->path);
        return 0;
    }if(file->lock!=-1){
        SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                       "ERROR - Mutex unlock of file failed, %s\n", file->path);
        return 1;
    }
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return -1;
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


