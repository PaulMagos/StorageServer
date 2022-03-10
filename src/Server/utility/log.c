//
// Created by paul on 26/01/22.
//

#include "../../../headers/log.h"

void createLog(char* dir, logFile* log){
    char* directory;
    if(dir == NULL) directory = "../../log/";
    else directory = dir;
    if(*log == NULL) *log = malloc(sizeof(logF));
    else return;
    if(!*log){
        fprintf(stderr, "ERROR - Log Structure alloc failure\n");
        exit(EXIT_FAILURE);
    }

    char* time;
    getTime(&time);
    if(time == NULL) return;
    int pathLen = strlen(directory) + strlen(time) + strlen(".txt") + 1;
    char* path = malloc(pathLen);
    if(!path) exit(ENOMEM);
    snprintf(path, pathLen, "%s%s.txt", directory,time);

    if(((*log)->file = fopen(path, "w")) == NULL){
        free(log);
        free(path);
        free(time);
        fprintf(stderr, "ERROR - Log File open failure\n");
        exit(errno);
    }
    free(path);
    free(time);

    if(pthread_mutex_init(&(*log)->mutex, NULL) != 0){
        fclose((*log)->file);
        free(log);
        fprintf(stderr, "ERROR - Log Mutex init failure\n");
        exit(EXIT_FAILURE);
    }

    fprintf(((*log)->file), "-------------------- LOG FILE --------------------\n");
    fflush((*log)->file);
}

int appendOnLog(logFile log, char* strBuf,...){
    if(!log) {
        errno = EINVAL;
        return -1;
    }
    char* time;
    getTime(&time);
    if(time == NULL) return -1;
    fprintf(log->file, "%s -> ", time);

    SYSCALL_RETURN(pthread_mutex_init, pthread_mutex_lock(&(log->mutex)),
                   "ERROR - Log File Lock, errno = %d\n", errno);

    va_list argList;
    va_start(argList, strBuf);
    vfprintf(log->file, strBuf, argList);
    fflush(log->file);
    va_end(argList);

    SYSCALL_RETURN(pthread_mutex_init, pthread_mutex_unlock(&(log->mutex)),
                   "ERROR - Log File UnLock, errno = %d\n", errno);
    free(time);
    return 0;
}

int closeLogStr(logFile log){
    if(log == NULL) return -1;
    fprintf(log->file, "-------------------- END LOG --------------------\n");
    int scRes;
    SYSCALL_EXIT(fclose, scRes, fclose(log->file), "ERROR - Log File Close, errno = %d\n", errno);
    pthread_mutex_destroy(&(log->mutex));
    free(log);
    return 0;
}

void logSeparator(logFile log){
    if(log == NULL) return;
    fprintf(log->file, "-------------------------------------------------\n");
}
