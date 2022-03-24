/*
 * Created by paul on 26/01/22.
 */

#include "../../../headers/log.h"

void createLog(char* dir, logFile* log){
    char* time;
    char* path;
    int pathLen;
    char* directory;

    if(dir == NULL) {
        directory = "../log";
    }
    else directory = dir;
    if(*log == NULL) *log = malloc(sizeof(logF));
    else return;
    if(!*log){
        fprintf(stderr, "ERROR - Log Structure alloc failure\n");
        exit(EXIT_FAILURE);
    }


    getTime(&time,0);
    if(time == NULL) return;
    pathLen = strlen(directory) + strlen(time) + strlen(".txt") + 2;
    path = malloc(pathLen);
    if(!path) exit(ENOMEM);
    snprintf(path, pathLen, "%s%c%s.txt", directory, ((directory[strlen(directory)-1] == '/')? 0:'/'),time);


    if(((*log)->file = fopen(path, "w")) == NULL){
        free((*log));
        free(path);
        free(time);
        fprintf(stderr, "ERROR - Log File open failure\n");
        exit(errno);
    }
    free(path);
    free(time);

    if(pthread_mutex_init(&(*log)->mutex, NULL) != 0){
        fclose((*log)->file);
        free(*log);
        fprintf(stderr, "ERROR - Log Mutex init failure\n");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&((*log)->mutex));
    fprintf(((*log)->file), "-------------------- LOG FILE --------------------\n");
    fflush((*log)->file);
    pthread_mutex_unlock(&((*log)->mutex));
}

int appendOnLog(logFile log, char* strBuf,...){
    char* time;
    va_list argList;
    if(!log || !strBuf) {
        errno = EINVAL;
        return -1;
    }
    SYSCALL_RETURN(pthread_mutex_init, pthread_mutex_lock(&(log->mutex)),
                   "ERROR - Log File Lock, errno = %d\n", errno);

    va_start(argList, strBuf);

    getTime(&time,1);
    if(time == NULL) return -1;
    fprintf(log->file, "%s ->\t", time);

    vfprintf(log->file, strBuf, argList);
    fflush(log->file);
    va_end(argList);

    SYSCALL_RETURN(pthread_mutex_init, pthread_mutex_unlock(&(log->mutex)),
                   "ERROR - Log File UnLock, errno = %d\n", errno);
    free(time);
    return 0;
}

int closeLogStr(logFile log){
    int scRes;
    if(log == NULL) return -1;
    fprintf(log->file, "-------------------- END LOG --------------------\n");
    SYSCALL_ASSIGN(fclose, scRes, fclose(log->file), "ERROR - Log File Close, errno = %d\n", errno);
    pthread_mutex_destroy(&(log->mutex));
    free(log);
    if(scRes==-1) return -1;
    return 0;
}

void logSeparator(logFile log){
    if(log == NULL) return;
    fprintf(log->file, "-------------------------------------------------\n");
}
