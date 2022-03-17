//
// Created by paul on 26/01/22.
//

#include "../../../headers/server.h"


int opExecute(int fd, int workerId, int mainPipe, operation op);
int lastOpUpdate(serverFile* file, fileFlags op);
int ExpelledHandler(int fd, int workerId, List expelled);

static inline int writeDataLength(int fd, int* len, void* buffer);

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
    operation operation1 = null;
    int readed;


    if((readed = readn(fd_client, &operation1,sizeof(operation))) == -1){
        errno = EBADMSG;
        if(writen(fd_client, &errno, sizeof(int)) == -1){
            fprintf(stderr,"[Thread %d]: ERROR: Read operation - Send error to %d, errno = %d\n", myId, fd_client, errno);
        }
        fprintf(stderr,"[Thread %d]: ERROR: Read operation - Read operation from %d, errno = %d\n", myId, fd_client, errno);
    }
    // If 0 fd disconnected
    if(readed == 0){
        // client non più connesso
        if(CloseConnection((int)fd_client, myId) == -1)
            fprintf(stderr, "[Thread %d]: ERROR - Closing connection to client fault, errno = %d", myId, errno);
        appendOnLog(ServerLog, "[Thread %d]: Client %d, connection closed\n", myId, fd_client);
        return;
    } else{
        if(opExecute(fd_client, myId, pipeM,operation1) == 1) {
            int success = 0;
            if(writen(fd_client, &success, sizeof(int)) == -1){
                fprintf(stderr, "[Thread %d]: ERROR - Sending success to %d, errno = %d", myId, fd_client, errno);
            }
        }
    }
    return;
}

int opExecute(int fd, int workerId, int mainPipe, operation op){
    int res;
    switch (op) {
        case of:{
            res = OpenFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 1;
            }
            break;
        }   // openFile
        case wr:{
            res = ReceiveFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 0;
            }
            break;
        }   // write
        case r:{
            res = SendFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 0;
            }
            break;
        }   // read file
        case rn:{
            res = SendNFiles(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 0;
            }
            break;
        }   // read nFiles
        case lk:{
            res = LockFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 1;
            }
            break;
        }   // lock file
        case unlk:{
            res = UnLockFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 1;
            }
            break;
        }   // unlock file
        case del:{
            res = DeleteFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 1;
            }
            break;
        }   // delete file
        case cl:{
            res = CloseFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 1;
            }
            break;
        }   // close file
        case ap:{
            res = AppendOnFile(fd, (int)workerId);
            if(res!=-1){
                if(writen(mainPipe, &fd, sizeof(int))==-1){
                    fprintf(stderr, "ERROR - Thread %d: writing on pipe", workerId);
                }
                return 0;
            }
            break;
        }   // append to file
        default:{
            res = -1;
            break;
        }
    }

    printf("%d\n", res);
    // Operation error error, send error to client
    if(writen((int)fd, &errno, sizeof(int )) == -1){
        fprintf(stderr, "[Thread %d]: ERROR - Sending info to client fault, errno = %d", workerId, errno);
    }
    if(errno == EBADMSG){
        appendOnLog(ServerLog, "[Thread %d]: Client %d, connection closed\n", workerId, fd);
        if(CloseConnection((int)fd, (int)workerId) == -1)
            fprintf(stderr, "[Thread %d]: ERROR - Closing connection to client fault, errno = %d", workerId, errno);
    }
    return -1;
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
            if(file->writers==0) {
                fileReadersDecrement(file);
                fileWritersIncrement(file);
            } else{
                if(file->lockFd != fd_client){
                    continue;
                }
            }
            if(!searchInt(file->clients_fd->head, fd_client)){
                pullTop(&file->clients_fd, NULL, NULL);
                if(file->lockFd==fd_client) file->lockFd = -1;
                fileWritersDecrement(file);
            }
        }
    }
    SYSCALL_RETURN(closeConn_pthread_mutex_lock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "Error - Server unLock failure, errno = %d", errno);

    close(fd_client);
    return 0;
}

int OpenFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;

    if(readn(fd_client, &(length), sizeof(int)) == -1) {
        fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }

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
                strncpy(File->path, buffer, length);
                createList(&File->clients_fd);
                pushTop(&File->clients_fd, NULL, &fd_client);
                if(pthread_mutex_init(&(File->lock), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_mutex_init(&(File->order), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_cond_init(&(File->condition), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    pthread_mutex_destroy(&(File->order));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(icl_hash_insert(ServerStorage->filesTable, File->path, File) == NULL){
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
                appendOnLog(ServerLog, "[Thread %d]: File %s created for client %d\n", workerId, File->path, fd_client);
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

                if(searchInt(File->clients_fd->head, fd_client) == 0){
                    pushTop(&File->clients_fd, NULL, &fd_client);
                } else{
                    free(buffer);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                File->lockFd = fd_client;
                SYSCALL_RETURN(pthread_unlock, pthread_mutex_unlock(&(File->lock)),
                               "ERROR - Unlock file %s failure", File->path)
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                fileWritersDecrement(File);
                lastOpUpdate(File, O_LOCK);
                appendOnLog(ServerLog,"[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
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
                createList(&File->clients_fd);
                if(pthread_mutex_init(&(File->lock), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_mutex_init(&(File->order), NULL) == -1){
                    free(buffer);
                    free(File->path);
                    pthread_mutex_destroy(&(File->lock));
                    free(File);
                    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
                    return -1;
                }
                if(pthread_cond_init(&(File->condition), NULL) == -1){
                    free(buffer);
                    free(File->path);
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
                appendOnLog(ServerLog,"[Thread %d]: File %s created by client %d\n", workerId, File->path, fd_client);
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
            if(!searchInt(File->clients_fd->head, fd_client))
                pushTop(&File->clients_fd, NULL, &fd_client);
            File->lockFd = fd_client;
            SYSCALL_RETURN(pthread_unlock, pthread_mutex_unlock(&(File->lock)),
                           "ERROR - Unlock file %s failure", File->path)
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock release failure, errno = %d", errno)
            fileWritersDecrement(File);
            lastOpUpdate(File, O_LOCK);
            appendOnLog(ServerLog,"[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
            free(buffer);
            return 0;
        }
        case (O_PEN):{
            if(File==NULL){
                free(buffer);
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                errno = ENOENT;
                return -1;
            }
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock release failure, errno = %d", errno)
            if(File->lockFd!=-1 && File->lockFd!=fd_client){
                free(buffer);
                SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                               "ERROR - ServerStorage lock release failure, errno = %d", errno)
                return -1;
            }

            fileWritersIncrement(File);
            if(searchInt(File->clients_fd->head, fd_client)==0){
                pushTop(&File->clients_fd, NULL, &fd_client);
            }
            fileWritersDecrement(File);
            appendOnLog(ServerLog,"[Thread %d]: File %s opened by client %d\n", workerId, File->path, fd_client);
            free(buffer);
            return 0;
        }
    }
    return -1;
}
int CloseFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
        fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }

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
    fileWritersIncrement(File);

    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(searchInt(File->clients_fd->head, fd_client)==1){
        void* data = NULL;
        pullTop(&File->clients_fd, NULL, &data);
    } else{
        free(buffer);
        fileWritersDecrement(File);
        return -1;
    }
    free(buffer);
    fileWritersDecrement(File);
    appendOnLog(ServerLog, "[Thread %d]: File %s closed by client %d\n", workerId, File->path, fd_client);
    return 0;
}
int DeleteFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }

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
    fileWritersIncrement(File);
    if(File->lockFd!=-1 && File->lockFd!=fd_client && File->clients_fd->len>0){
        free(buffer);
        errno = EACCES;
        fileWritersDecrement(File);
        return -1;
    }
    size_t size = File->size;
    if(icl_hash_delete(ServerStorage->filesTable, buffer, free, freeFile) == -1){
        free(buffer);
        fileWritersDecrement(File);
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }
    fileBytesDecrement((size_t)size);
    fileNumDecrement();
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    appendOnLog(ServerLog,"[Thread %d]: File %s deleted by client %d\n", workerId, buffer, fd_client);
    free(buffer);
    return 0;
}
int ReceiveFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }


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

    if(File->content!=NULL || searchInt(File->clients_fd->head, fd_client)==0 ||
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


    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }

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
        file->toDelete = 1;
        pushTop(&expelled, file->path, NULL);
    }

    fileBytesIncrement((size_t)length);
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    File->content = malloc(length);
    memcpy(File->content, buffer, length);
    free(buffer);
    File->latsOp = O_WRITE;
    File->size = (size_t)length;
    fileWritersDecrement(File);
    // Success till here
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
    }
    appendOnLog(ServerLog, "[Thread %d]: File %s received from client %d\n", workerId, File->path, fd_client);
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
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }


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
    if((File->lockFd != -1 && File->lockFd != fd_client)){
        errno = EACCES;
        fileReadersDecrement(File);
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    if(searchInt(File->clients_fd->head, fd_client)==0){
        errno = EACCES;
        fileReadersDecrement(File);
        return -1;
    }
    // File Found, say it to Client
    int success = 0;
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
    }


    if(writen(fd_client, &File->size, sizeof(int)) == -1) {
        fprintf(stderr, "ERROR - Sending length to %d, errno = %d", fd_client, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    if(File->size!=0){
        if(writen( fd_client, File->content, File->size) == -1) {
            fprintf(stderr, "ERROR - Sending content to %d, errno = %d", fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }
    //writeDataLength(fd_client, , &buffer);
    fileReadersDecrement(File);
    lastOpUpdate(File, O_READ);
    appendOnLog(ServerLog, "[Thread %d]: File %s sent to client %d\n", workerId, File->path, fd_client);
    return 0;

}
int SendNFiles(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int readed;
    int numberOfFiles;
    if((readed = readn(fd_client, &numberOfFiles, sizeof(int))) == -1){
        fprintf(stderr, "ERROR - Reading number of files to send from client %d", fd_client);
        errno = EBADMSG;
        return -1;
    }
    SYSCALL_RETURN(pthred_lock, pthread_mutex_lock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    numberOfFiles = (ServerStorage->actualFilesNumber<numberOfFiles)? ServerStorage->actualFilesNumber:numberOfFiles;
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
                fprintf(stderr, "ERROR - List push failure for sendNFiles, %d", errno);
                fileReadersDecrement(file);
                return -1;
            }
        }
    }

    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock release failure, errno = %d", errno)
    // Success till here, say it to Client
    int success = 0;
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d\n", fd_client, errno);
    }

    appendOnLog(ServerLog, "[Thread %d]: Send %d files to client %d\n", workerId, numberOfFiles, fd_client);
    if(ExpelledHandler(fd_client, workerId, expelled)!=0){
        deleteList(&expelled);
        errno = EBADMSG;
        return -1;
    }
    return 0;
}
int LockFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }
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
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(searchInt(File->clients_fd->head, fd_client)==0){
        errno = EACCES;
        fileWritersDecrement(File);
        return -1;
    }
    if(File->lockFd!=-1){
        if(File->lockFd != fd_client){
            errno = EBUSY;
            fileWritersDecrement(File);
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return -1;
        }
    }
    File->lockFd = fd_client;
    fileWritersDecrement(File);
    lastOpUpdate(File,O_LOCK);
    appendOnLog(ServerLog, "[Thread %d]: File %s locked by client %d\n", workerId, File->path, fd_client);
    return 0;
}
int UnLockFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }
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
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
    if(searchInt(File->clients_fd->head, fd_client)==0){
        errno = EACCES;
        fileWritersDecrement(File);
        return -1;
    }
    if(File->lockFd!=-1){
        if(File->lockFd != fd_client){
            errno = EBUSY;
            fileWritersDecrement(File);
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return -1;
        }
    }
    if(File->lockFd==-1){
        errno = EPERM;
        fileWritersDecrement(File);
        SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                       "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
        return -1;
    }
    File->lockFd = -1;
    fileWritersDecrement(File);
    lastOpUpdate(File,O_UNLOCK);
    appendOnLog(ServerLog, "[Thread %d]: File %s unlocked by client %d\n", workerId, File->path, fd_client);
    return 0;
}
int AppendOnFile(int fd_client, int workerId){
    if(fd_client == -1 || workerId == -1) {
        errno = EINVAL;
        return -1;
    }

    int length;
    void* buffer;
    if(readn(fd_client, &(length), sizeof(int)) == -1) {
                fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);

        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }

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

    if(searchInt(File->clients_fd->head, fd_client)==0 || (File->lockFd!=-1 && File->lockFd!=fd_client)){
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


    if(readn(fd_client, &(length), sizeof(int)) == -1) {
        fprintf(stderr, "[Thread %d]: ERROR - Reading length from %d, errno = %d", workerId, fd_client, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(length);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(length!=0){
        if( readn( fd_client, buffer, length) == -1) {
            fprintf(stderr, "[Thread %d]: ERROR - Reading content from %d, errno = %d", workerId, fd_client, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }

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
    while (ServerStorage->actualFilesBytes-File->size+(size_t)length > ServerConfig.maxByte){
        serverFile* file = icl_hash_toReplace(ServerStorage->filesTable, ServerConfig.policy);
        if(file == NULL) {
            free(buffer);
            fileWritersDecrement(File);
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return -1;
        }
        file->toDelete = 1;
        pushTop(&expelled, file->path, NULL);
    }

    fileBytesIncrement((size_t)length);
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)

    if(File->content!=NULL) free(File->content);
    File->content = malloc(sizeof(length)+1);
    memcpy(File->content, buffer, length+1);
    free(buffer);
    File->size = (size_t)length;
    fileWritersDecrement(File);
    lastOpUpdate(File, O_APPND);
    // Success till here
    if(writen(fd_client, &success, sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending success to %d, errno = %d", fd_client, errno);
    }
    appendOnLog(ServerLog, "[Thread %d]: File %s updated by client %d\n", workerId, File->path, fd_client);
    if(ExpelledHandler(fd_client, workerId, expelled)==-1){
        deleteList(&expelled);
        errno = EBADMSG;
        return -1;
    }
    return 0;
}

int ExpelledHandler(int fd, int workerId, List expelled){
    if(expelled==NULL) return -1;
    if(writen(fd, &(expelled->len), sizeof(int)) == -1){
        fprintf(stderr, "ERROR - Sending number of files to %d", fd);
        return -1;
    }
    int num = 0;

    char* index;
    int len;
    serverFile* File;
    while (expelled->len!=0){
        pullTop(&expelled, &index, NULL);
        File = icl_hash_find(ServerStorage->filesTable, index);
        if(File==NULL){
            errno = ENOENT;
            SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                           "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
            return -1;
        }
        len = strlen(index)+1;

        if(writen(fd, &len, sizeof(int)) == -1) {
            fprintf(stderr, "ERROR - Sending length to %d, errno = %d", fd, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
        if(len!=0){
            if(writen(fd, &index, len) == -1) {
                fprintf(stderr, "ERROR - Sending content to %d, errno = %d", fd, EBADMSG);
                errno = EBADMSG;
                return -1;
            }
        }
        //writeDataLength(fd, &len, &index);

        fileWritersIncrement(File);
        len = (int)File->size;

        if(writen(fd, &len, sizeof(int)) == -1) {
            fprintf(stderr, "ERROR - Sending length to %d, errno = %d", fd, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
        if(len!=0){
            if(writen(fd, &File->content, len) == -1) {
                fprintf(stderr, "ERROR - Sending content to %d, errno = %d", fd, EBADMSG);
                errno = EBADMSG;
                return -1;
            }
        }
        //writeDataLength(fd, &len, &File->content);

        num++;
        fileWritersDecrement(File);
        if(File->toDelete==1) {
            icl_hash_delete(ServerStorage->filesTable, index, free, freeFile);
            ServerStorage->expelledFiles++;
            appendOnLog(ServerLog, "[Thread %d]: File %s expelled, of %d files sent to client %d\n", workerId, index, num,fd);
        }
        else {
            lastOpUpdate(File, O_READ);
            appendOnLog(ServerLog, "[Thread %d]: File %s, of %d files sent to client %d\n", workerId, index, num, fd);
        }
        memset(index, 0, sizeof (char ) * strlen(index));
    }
    SYSCALL_RETURN(pthred_unlock, pthread_mutex_unlock(&(ServerStorage->lock)),
                   "ERROR - ServerStorage lock acquisition failure, errno = %d", errno)
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

/** Sends data to the server, from len and buffer
 *
 *   \retval -1   error (errno settato)
 *   \retval  0   if success
 */
static inline int writeDataLength(int fd, int* len, void* buffer){
    if(!(buffer) || *len < 0) {
        errno = EINVAL;
        return -1;
    }
    if(writen((long)fd, &len, sizeof(int)) == -1) {
        fprintf(stderr, "ERROR - Sending length to %d, errno = %d", fd, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    if(len!=0){
        if(writen((long)fd, &(buffer), *len) == -1) {
            fprintf(stderr, "ERROR - Sending content to %d, errno = %d", fd, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }
    return 0;
}