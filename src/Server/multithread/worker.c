//
// Created by paul on 26/01/22.
//

#include "../../../headers/server.h"


int opExecute(int fd, int workerId, message* message1);
int lastOpUpdate(serverFile* file, fileFlags op);
int ExpelledHandler(int fd, int workerId, List expelled);


void taskExecute(void* argument){
    if( argument == NULL ) {
        fprintf(stderr, "ERROR - Invalid Worker Argument, errno = %d", EINVAL);
        exit(EXIT_FAILURE);
    }

    wTask *tmpArgs = argument;
    int myId = tmpArgs->worker_id;
    int fd_client = (int)tmpArgs->client_fd;
    int pipeM = tmpArgs->pipeT;
    free(argument);
    int r = -1;
    if(writen(pipeM, &r, sizeof(int))==-1){
        fprintf(stderr, "ERROR - Thread %d: writing on pipe", myId);
    }


    appendOnLog(ServerLog, "[Thread %d]: Client %d Task\n", myId, fd_client);
    if(ServerStorage->stdOutput) printf("[Thread %d]: Client %d Task\n", myId, fd_client);

    message message1;
    memset(&message1, 0, sizeof(message));

    if(readMessage(fd_client, &message1) == -1){
        appendOnLog(ServerLog, "[Thread %d]: Client %d, connection closed\n", myId, fd_client);
        if(ServerStorage->stdOutput) printf("[Thread %d]: Client %d connection closed\n", myId, fd_client);
        if(CloseConnection((int)fd_client, (int)myId) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Closing connection to client fault, errno = %d", myId, errno);
        }
        freeMessageContent(&message1);
        return;
    }
    if(opExecute(fd_client, myId, &message1)==0){
        if(writeMessage(fd_client, &message1)==-1){
            if(ServerStorage->stdOutput)
                printf("[Thread %d]: FATAL ERROR, writing message to client %d", myId, fd_client);
            appendOnLog(ServerLog, "[Thread %d]: FATAL ERROR, writing message to client %d", myId, fd_client);
            freeMessageContent(&message1);
            return;
        }else {
            if(message1.feedback==ERROR){
                if(ServerStorage->stdOutput)
                    printf("[Thread %d]: Client %d request %s failed - %s\n", myId, fd_client,
                            requestToString(message1.request), strerror(message1.additional));
                appendOnLog(ServerLog, "[Thread %d]: Client %d request %s failed - %s\n", myId, fd_client,
                            requestToString(message1.request), strerror(message1.additional));
            }
            if (writen(pipeM, &fd_client, sizeof(int)) == -1) {
                fprintf(stderr, "ERROR - Thread %d: writing on pipe", myId);
            }
            freeMessageContent(&message1);
            return;
        }
    }
    appendOnLog(ServerLog, "[Thread %d]:  OP to client %d", myId, fd_client);

    freeMessageContent(&message1);
    return;
}

int opExecute(int fd, int workerId, message* message1){
    switch (message1->request) {
        case O_WRITE:{
            ReceiveFile(fd, workerId, message1);
            break;
        }   // write
        case O_READ:{
            SendFile(fd, workerId, message1);
            break;
        }   // read file
        case O_READN:{
            SendNFiles(fd, workerId, message1);
            break;
        }   // read nFiles
        case (O_LOCK):{
            LockFile(fd, workerId, message1);
            break;
        }   // lock file
        case O_UNLOCK:{
            UnLockFile(fd, workerId, message1);
            break;
        }   // unlock file
        case O_DEL:{
            DeleteFile(fd, workerId, message1);
            break;
        }   // delete file
        case O_CLOSE:{
            CloseFile(fd, workerId, message1);
            break;
        }   // close file
        case O_APPND:{
            AppendOnFile(fd, workerId, message1);
            break;
        }   // append to file
        case O_CREAT:
        case O_CREAT_LOCK:
        case O_PEN:{
            OpenFile(fd, workerId, message1);
            break;
        }   // openFile
        default:{
            message1->additional = EBADMSG;
            message1->feedback = ERROR;
            return -1;
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

    if(ServerStorage->actualFilesNumber>0) {
        serverFile *file;
        icl_entry_t *bucket, *curr;
        for (int i = 0; i < ServerStorage->filesTable->nbuckets; i++) {
            bucket = ServerStorage->filesTable->buckets[i];
            for (curr = bucket; curr != NULL; curr = curr->next) {
                file = ((serverFile *) curr->data);
                if (file->writers != 0) {
                    continue;
                }
                fileWritersIncrement(file);
                if (!searchInt(file->clients_fd->head, fd_client)) {
                    pullTop(&file->clients_fd, NULL, NULL);
                    if (file->lockFd == fd_client) file->lockFd = -1;
                    fileWritersDecrement(file);
                }
            }
        }
    }
    SYSCALL_RETURN(closeConn_pthread_mutex_lock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "Error - Server unLock failure, errno = %d", errno);

    close(fd_client);
    return 0;
}

void OpenFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);

    switch (message1->request) {
        case O_CREAT:{
            if(File!=NULL) {
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = EEXIST;
                message1->feedback = ERROR;
                message1->additional = errno;
                return;
            }
            else{
                if(ServerStorage->actualFilesNumber == ServerConfig.maxFile){
                    serverFile* expelled = icl_hash_toReplace(ServerStorage->filesTable, ServerConfig.policy);
                    fileWritersIncrement(expelled);
                    expelled->toDelete = fd_client;
                    appendOnLog(ServerLog, "[Thread %d]: File %s deleted for new %s file by client %d\n", workerId, expelled->path, message1->content, fd_client);
                    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s deleted for new %s file by client %d\n", workerId, expelled->path, (char*)message1->content, fd_client);
                    fileWritersDecrement(expelled);
                    fileBytesDecrement(expelled->size);
                    fileNumDecrement();
                }
                File = malloc(sizeof(serverFile));
                if(!File) {
                    message1->feedback = ERROR;
                    message1->additional = ENOMEM;
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->creationTime))==-1){
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->lastOpTime))==-1){
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                File->path = malloc(message1->size);
                if(!(File->path)){
                    freeMessage(message1);
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                strncpy(File->path, message1->content, message1->size);
                createList(&File->clients_fd);
                pushTop(&File->clients_fd, NULL, &fd_client);
                if(pthread_mutex_init(&(File->lock), NULL) == -1){
                    free(File->path);
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(pthread_mutex_init(&(File->order), NULL) == -1){
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(pthread_cond_init(&(File->condition), NULL) == -1){
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    pthread_mutex_destroy(&(File->order));
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(icl_hash_insert(ServerStorage->filesTable, File->path, File) == NULL){
                    fprintf(stderr, "ERROR - Insert file %s failure", File->path);
                    freeFile(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                File->size = 0;
                File->writers = 0;
                File->readers = 0;
                File->lockFd = -1;
                File->toDelete = 0;
                File->content = NULL;
                File->latsOp = O_CREAT;
                fileNumIncrement();
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno);
                message1->feedback=SUCCESS;
                if(ServerStorage->stdOutput) printf("[Thread %d]: File %s created for client %d\n", workerId, File->path, fd_client);
                appendOnLog(ServerLog, "[Thread %d]: File %s created for client %d\n", workerId, File->path, fd_client);
                return;
            }
            break;
        }
        case O_LOCK:{
            if(File!=NULL){
                if(isLocked(File, fd_client) == 0){
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    errno = EDEADLK;
                    message1->additional = errno;
                    message1->feedback = ERROR;
                    return;
                }
                if(isLocked(File, fd_client) == 1){
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    errno = EBUSY;
                    message1->additional = errno;
                    message1->feedback = ERROR;
                    return;
                }
                fileWritersIncrement(File);
                SYSCALL_RET(pthread_lock, pthread_mutex_lock(&(File->lock)),
                               "ERROR - Lock file %s failure", File->path)

                if(searchInt(File->clients_fd->head, fd_client) == 0){
                    pushTop(&File->clients_fd, NULL, &fd_client);
                } else{
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                File->lockFd = fd_client;
                SYSCALL_RET(pthread_unlock, pthread_mutex_unlock(&(File->lock)),
                               "ERROR - Unlock file %s failure", File->path)
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                fileWritersDecrement(File);
                lastOpUpdate(File, O_LOCK);
                message1->feedback = SUCCESS;
                if(ServerStorage->stdOutput) printf("[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
                appendOnLog(ServerLog,"[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
                return;
            } else{
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = ENOENT;
                message1->additional = errno;
                message1->feedback = ERROR;
                return;
            }
        }
        case (O_CREAT_LOCK):{
            if(File==NULL) {
                if(ServerStorage->actualFilesNumber==ServerConfig.maxFile){
                    serverFile* expelled = icl_hash_toReplace(ServerStorage->filesTable, ServerConfig.policy);
                    fileWritersIncrement(expelled);
                    expelled->toDelete = fd_client;
                    appendOnLog(ServerLog, "[Thread %d]: File %s deleted for new %s file by client %d\n", workerId, expelled->path, message1->content, fd_client);
                    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s deleted for new %s file by client %d\n", workerId, expelled->path, (char*)message1->content, fd_client);
                    fileWritersDecrement(expelled);
                    fileBytesDecrement(expelled->size);
                    fileNumDecrement();
                }
                File = malloc(sizeof(serverFile));
                if(!File) {
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->creationTime))==-1){
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(clock_gettime(CLOCK_REALTIME, &(File->lastOpTime))==-1){
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                File->path = malloc(message1->size);
                if(!(File->path)){
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                strncpy(File->path, message1->content, message1->size);
                freeMessageContent(message1);
                createList(&File->clients_fd);
                if(pthread_mutex_init(&(File->lock), NULL) == -1){
                    free(File->path);
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(pthread_mutex_init(&(File->order), NULL) == -1){
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                if(pthread_cond_init(&(File->condition), NULL) == -1){
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    pthread_mutex_destroy(&(File->order));
                    free(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno);
                    return;
                }
                if(icl_hash_insert(ServerStorage->filesTable, File->path, File) == NULL){
                    fprintf(stderr, "ERROR - Insert file %s failure", File->path);
                    freeFile(File);
                    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return;
                }
                File->size = 0;
                File->writers = 0;
                File->readers = 0;
                File->lockFd = -1;
                File->toDelete = 0;
                File->content = NULL;
                File->latsOp = O_CREAT;
                fileNumIncrement();
                if(ServerStorage->stdOutput) printf("[Thread %d]: File %s created by client %d\n", workerId, File->path, fd_client);
                appendOnLog(ServerLog,"[Thread %d]: File %s created by client %d\n", workerId, File->path, fd_client);
            }
            if(isLocked(File, fd_client) == -1){
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = EDEADLK;
                message1->additional = errno;
                message1->feedback = ERROR;
                return;
            }
            if(isLocked(File, fd_client) == 1){
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = EBUSY;
                message1->additional = errno;
                message1->feedback = ERROR;
                return;
            }
            fileWritersIncrement(File);
            SYSCALL_RET(pthread_lock, pthread_mutex_lock(&(File->lock)),
                           "ERROR - Lock file %s failure", File->path)
            if( searchInt(File->clients_fd->head, fd_client) == 0)
                pushTop(&File->clients_fd, NULL, &fd_client);
            File->lockFd = fd_client;
            SYSCALL_RET(pthread_unlock, pthread_mutex_unlock(&(File->lock)),
                           "ERROR - Unlock file %s failure", File->path)
            SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock release failure, errno = %d", errno)
            fileWritersDecrement(File);
            lastOpUpdate(File, O_LOCK);
            message1->feedback = SUCCESS;
            if(ServerStorage->stdOutput) printf("[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
            appendOnLog(ServerLog,"[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
            return;
        }
        case (O_PEN):{
            if(File==NULL){
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = ENOENT;
                message1->additional = errno;
                message1->feedback = ERROR;
                return ;
            }
            SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock release failure, errno = %d", errno)
            if(isLocked(File, fd_client)==1){
                errno = EBUSY;
                message1->additional = errno;
                message1->feedback = ERROR;
                SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                return;
            }

            fileWritersIncrement(File);
            if(searchInt(File->clients_fd->head, fd_client)==0){
                pushTop(&File->clients_fd, NULL, &fd_client);
            }
            fileWritersDecrement(File);
            message1->feedback = SUCCESS;
            if(ServerStorage->stdOutput) printf("[Thread %d]: File %s opened by client %d\n", workerId, File->path, fd_client);
            appendOnLog(ServerLog,"[Thread %d]: File %s opened by client %d\n", workerId, File->path, fd_client);
            return ;
        }
        default: return;
    }
    return;
}
void CloseFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    fileWritersIncrement(File);

    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(searchInt(File->clients_fd->head, fd_client)==1){
        void* data = NULL;
        pullTop(&File->clients_fd, NULL, &data);
    } else{
        message1->feedback = ERROR;
        message1->additional = 13;
        fileWritersDecrement(File);
        return;
    }
    message1->feedback = SUCCESS;
    fileWritersDecrement(File);
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s closed by client %d\n", workerId, File->path, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: File %s closed by client %d\n", workerId, File->path, fd_client);
    return;
}
void DeleteFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }


    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return ;
    }
    fileWritersIncrement(File);
    if(isLocked(File, fd_client)!=0 && File->clients_fd->len>0){
        errno = EACCES;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                    "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        fileWritersDecrement(File);
        return ;
    }
    if(icl_hash_delete(ServerStorage->filesTable, message1->content, free, freeFile) == -1){
        message1->additional = 132;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    fileBytesDecrement(File->size);
    fileNumDecrement();
    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno);
    message1->feedback = SUCCESS;
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s deleted by client %d\n", workerId, (char*)message1->content, fd_client);
    appendOnLog(ServerLog,"[Thread %d]: File %s deleted by client %d\n", workerId, message1->content, fd_client);
    //freeMessageContent(message1);
    return;
}
void ReceiveFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return ;
    }
    fileWritersIncrement(File);

    if(File->content!=NULL || searchInt(File->clients_fd->head, fd_client)==0 ||
            isLocked(File, fd_client)==1){
        // File already exists, use append to add new content
        fileWritersDecrement(File);
        message1->additional = EACCES;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return ;
    }


    freeMessageContent(message1);
    message1->feedback = SUCCESS;
    writeMessage(fd_client, message1);

    readMessage(fd_client, message1);

    if(message1->request!=O_DATA){
        message1->additional = errno;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        freeMessageContent(message1);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return ;
    }

    if( message1->size > ServerConfig.maxByte){
        errno = EFBIG;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        freeMessageContent(message1);
        icl_hash_delete(ServerStorage->filesTable, File->path, free, freeFile);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return ;
    }
    List expelled;
    createList(&expelled);
    while (ServerStorage->actualFilesBytes+message1->size > ServerConfig.maxByte){
        serverFile* file = icl_hash_toReplace(ServerStorage->filesTable, ServerConfig.policy);
        if(file == NULL) {
            message1->additional = 132;
            message1->feedback = ERROR;
            fileWritersDecrement(File);
            freeMessageContent(message1);
            icl_hash_delete(ServerStorage->filesTable, File->path, free, freeFile);
            SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return ;
        }
        fileBytesDecrement(file->size);
        fileNumDecrement();
        file->toDelete = 1;
        pushTop(&expelled, file->path, NULL);
    }

    fileBytesIncrement(message1->size);

    File->size = message1->size;
    File->content = malloc(message1->size);
    memcpy(File->content, message1->content, message1->size);

    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                "ERROR - ServerStorage lock acquisition failure, errno = %d", errno);
    icl_hash_toDelete(ServerStorage->filesTable, expelled, fd_client);

    message1->feedback = SUCCESS;
    message1->additional = expelled->len;
    writeMessage(fd_client, message1);

    fileWritersDecrement(File);
    lastOpUpdate(File, O_WRITE);
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s received by client %d\n", workerId, File->path, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: File %s received from client %d\n", workerId, File->path, fd_client);
    if(ExpelledHandler(fd_client, workerId, expelled)==-1){
        deleteList(&expelled);
        errno = EBADMSG;
        message1->additional = errno;
        message1->feedback = ERROR;
        freeMessageContent(message1);
        return ;
    }
    message1->feedback = SUCCESS;
    message1->request = O_WRITE;
    return ;
}
void SendFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    freeMessageContent(message1);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    fileReadersIncrement(File);

    if((File->lockFd != -1 && File->lockFd != fd_client)){
        errno = EACCES;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileReadersDecrement(File);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    if(searchInt(File->clients_fd->head, fd_client)==0){
        errno = EACCES;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileReadersDecrement(File);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                    "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    message1->feedback = SUCCESS;
    writeMessage(fd_client, message1);

    message1->content = malloc(File->size);
    memcpy(message1->content, File->content, File->size);
    message1->size = File->size;
    message1->feedback = SUCCESS;
    message1->request = O_DATA;

    fileReadersDecrement(File);
    lastOpUpdate(File, O_READ);
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s sent to client %d\n", workerId, File->path, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: File %s sent to client %d\n", workerId, File->path, fd_client);
    return;

}
void SendNFiles(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    int numberOfFiles = 0;
    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    numberOfFiles = (ServerStorage->actualFilesNumber<message1->additional)? ServerStorage->actualFilesNumber:message1->additional;
    List expelled;
    createList(&expelled);

    icl_entry_t *bucket, *curr;
    serverFile* file;
    for(int i = 0; i < ServerStorage->filesTable->nbuckets && (int)expelled->len<numberOfFiles; i++){
        bucket = ServerStorage->filesTable->buckets[i];
        for(curr = bucket; curr!=NULL && (int)expelled->len<numberOfFiles; curr=curr->next){
            file = (serverFile*) curr->data;
            fileReadersIncrement(file);
            if(file->lockFd!=-1 && file->lockFd!=fd_client) {
                fileReadersDecrement(file);
                continue;
            }
            if(pushTop(&expelled, file->path, NULL) == -1){
                message1->additional = errno;
                message1->feedback = ERROR;
                fprintf(stderr, "ERROR - List push failure for sendNFiles, %d", errno);
                fileReadersDecrement(file);
                return;
            }
            fileReadersDecrement(file);
        }
    }

    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock release failure, errno = %d", errno)

                   numberOfFiles = expelled->len;
    emptyMessage(message1);
    message1->feedback = SUCCESS;
    message1->additional = expelled->len;
    writeMessage(fd_client, message1);
    if(ServerStorage->stdOutput) printf("[Thread %d]: Send %d files to client %d\n", workerId, numberOfFiles, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: Send %d files to client %d\n", workerId, numberOfFiles, fd_client);
    message1->additional = numberOfFiles;
    if(ExpelledHandler(fd_client, workerId, expelled)!=0){
        deleteList(&expelled);
        errno = EBADMSG;
        message1->additional = errno;
        message1->feedback = ERROR;
        return ;
    }
    message1->feedback = SUCCESS;
    return;
}
void LockFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    fileWritersIncrement(File);
    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(searchInt(File->clients_fd->head, fd_client)==0){
        errno = EACCES;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        return;
    }
    if(File->lockFd!=-1){
        if(File->lockFd != fd_client){
            errno = EBUSY;
            message1->additional = errno;
            message1->feedback = ERROR;
            fileWritersDecrement(File);
            return;
        }
    }
    File->lockFd = fd_client;
    fileWritersDecrement(File);
    lastOpUpdate(File,O_LOCK);
    message1->feedback = SUCCESS;
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
    return;
}
void UnLockFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    fileWritersIncrement(File);
    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(searchInt(File->clients_fd->head, fd_client)==0){
        errno = EACCES;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        return;
    }
    if(File->lockFd!=-1){
        if(File->lockFd != fd_client){
            errno = EBUSY;
            message1->additional = errno;
            message1->feedback = ERROR;
            fileWritersDecrement(File);
            return;
        }
    }
    if(File->lockFd==-1){
        errno = EPERM;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        return;
    }
    File->lockFd = -1;
    fileWritersDecrement(File);
    lastOpUpdate(File,O_UNLOCK);
    message1->feedback = SUCCESS;
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s unlocked by client %d\n", workerId, File->path, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: File %s unlocked by client %d\n", workerId, File->path, fd_client);
    return;
}
void AppendOnFile(int fd_client, int workerId, message* message1){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }

    SYSCALL_RET(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    serverFile* File = icl_hash_find(ServerStorage->filesTable, message1->content);
    if(File==NULL){
        errno = ENOENT;
        message1->additional = errno;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }
    fileWritersIncrement(File);

    if(searchInt(File->clients_fd->head, fd_client)==0 || (File->lockFd!=-1 && File->lockFd!=fd_client)){
        // File already exists, use append to add new content
        fileWritersDecrement(File);
        message1->additional = EPERM;
        message1->feedback = ERROR;
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }

    emptyMessage(message1);
    message1->feedback = SUCCESS;
    writeMessage(fd_client, message1);

    readMessage(fd_client, message1);
    if(message1->request!=O_DATA){
        message1->additional = 132;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }

    if( message1->size > ServerConfig.maxByte){
        errno = EFBIG;
        message1->additional = errno;
        message1->feedback = ERROR;
        fileWritersDecrement(File);
        SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return;
    }

    List expelled;
    createList(&expelled);
    while (ServerStorage->actualFilesBytes-File->size+message1->size > ServerConfig.maxByte){
        serverFile* file = icl_hash_toReplace(ServerStorage->filesTable, ServerConfig.policy);
        if(file == NULL) {
            message1->additional = 132;
            message1->feedback = ERROR;
            fileWritersDecrement(File);
            SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return;
        }
        fileBytesDecrement(file->size);
        fileNumDecrement();
        file->toDelete = 1;
        pushTop(&expelled, file->path, NULL);
    }

    fileBytesIncrement(message1->size);
    SYSCALL_RET(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    File->size = message1->size;
    File->content = malloc(message1->size+1);
    if(File->content!=NULL) free(File->content);
    memcpy(File->content, message1->content, message1->size);
    fileWritersDecrement(File);
    lastOpUpdate(File, O_APPND);
    // Success till here
    emptyMessage(message1);
    message1->feedback = SUCCESS;
    message1->additional = expelled->len;
    writeMessage(fd_client, message1);
    if(ServerStorage->stdOutput) printf("[Thread %d]: File %s updated by client %d\n", workerId, File->path, fd_client);
    appendOnLog(ServerLog, "[Thread %d]: File %s updated by client %d\n", workerId, File->path, fd_client);
    if(ExpelledHandler(fd_client, workerId, expelled)==-1){
        deleteList(&expelled);
        errno = EBADMSG;
        message1->additional = errno;
        message1->feedback = ERROR;
        return;
    }
    message1->feedback = SUCCESS;
    return;
}

int ExpelledHandler(int fd, int workerId, List expelled){
    if(expelled==NULL) return -1;
    if(expelled->len==0){
        deleteList(&expelled);
        return 0;
    }
    message curr;
    memset(&curr, 0, sizeof(message));
    int num = 0;
    char* index;
    int len;
    serverFile* File;
    while (expelled->len!=0){

        SYSCALL_RETURN(pthred_unlock, pthread_mutex_lock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        pullTop(&expelled, &index, NULL);
        File = icl_hash_find(ServerStorage->filesTable, index);
        if(File==NULL){
            errno = ENOENT;
            freeMessage(&curr);
            free(index);
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return -1;
        }
         len = strlen(index)+1;

        curr.size = len;
        curr.request = O_SEND;
        curr.content = malloc(len);
        strncpy(curr.content, index, len);
        writeMessage(fd, &curr);

        freeMessageContent(&curr);

        readMessage(fd, &curr);
        if(curr.feedback != SUCCESS) {
            curr.feedback = ERROR;
            curr.additional = 132;
            writeMessage(fd, &curr);
            free(index);
            return -1;
        }

        freeMessageContent(&curr);

        fileWritersIncrement(File);
        len = (int)File->size;

        curr.size = len;
        curr.content = malloc(len);
        memcpy(curr.content, File->content, len);
        curr.request = O_DATA;
        curr.feedback = SUCCESS;
        writeMessage(fd, &curr);

        num++;

        fileWritersDecrement(File);
        if(File->toDelete==1) {
            ServerStorage->expelledBytes+=len;
            icl_hash_delete(ServerStorage->filesTable, index, free, freeFile);
            ServerStorage->expelledFiles++;
            if(ServerStorage->stdOutput) printf("[Thread %d]: File %s expelled, of %d files sent to client %d\n", workerId, index, num,fd);
            appendOnLog(ServerLog, "[Thread %d]: File %s expelled, of %d files sent to client %d\n", workerId, index, num,fd);
        }
        else {
            lastOpUpdate(File, O_READ);
            if(ServerStorage->stdOutput) printf("[Thread %d]: File %s, of %d files sent to client %d\n", workerId, index, num, fd);
            appendOnLog(ServerLog, "[Thread %d]: File %s, of %d files sent to client %d\n", workerId, index, num, fd);
        }
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        freeMessageContent(&curr);
        free(index);
    }
    deleteList(&expelled);
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
    if(ServerStorage->stdOutput) fprintf(stdout, "READ Increment: File %s\n", file->path);
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
    if(ServerStorage->stdOutput) fprintf(stdout, "READ Decrement: File %s\n", file->path);
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
    if(ServerStorage->stdOutput) fprintf(stdout, "WRITE Increment: File %s lock\n", file->path);
    return 0;
}
int fileWritersDecrement(serverFile* file){
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    SYSCALL_RETURN(cond_broad, pthread_cond_broadcast(&(file->condition)),
                   "ERROR - Condition broadcast reader release file %s", file->path);
    file->writers--;
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    if(ServerStorage->stdOutput) fprintf(stdout, "WRITE Decrement: File %s lock release\n", file->path);
    return 0;
}
int isLocked(serverFile* file, long fd) {
    SYSCALL_RETURN(lock, pthread_mutex_lock(&file->lock), "ERROR - Mutex lock of file failed, %s\n", file->path);
    if(file->lockFd == fd){
        SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                       "ERROR - Mutex unlock of file failed, %s\n", file->path);
        return 0;
    }if(file->lockFd!=-1){
        SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                       "ERROR - Mutex unlock of file failed, %s\n", file->path);
        return 1;
    }
    SYSCALL_RETURN(unlock, pthread_mutex_unlock(&(file->lock)),
                   "ERROR - Mutex unlock of file failed, %s\n", file->path);
    return -1;
}
int lastOpUpdate(serverFile* file, fileFlags op){
    fileWritersIncrement(file);
    file->latsOp = op;
    if(clock_gettime(CLOCK_REALTIME, &(file->lastOpTime))==-1){
        return -1;
    }
    fileWritersDecrement(file);
    return 0;
}

wTask* taskCreate (int client_fd){
    if(client_fd == -1) return NULL;
    wTask *task;
    task = (wTask*)malloc(sizeof(wTask));
    task->pipeT = -1;
    task->worker_id = -1;
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
