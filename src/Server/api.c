//
// Created by paul on 16/01/22.
//
#include <time.h>
#include "errno.h"

#include "../../headers/api.h"


int openConnection(const char* sockname, int msec, const struct timespec abstime){
    errno = EINVAL;
    return -1;
}
/*
 *
 *
 *
 *
 * */

int closeConnection(const char* sockname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

int openFile(const char* pathname, int flags){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int readFile(const char* pathname, void** buf, size_t* size){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int readNFiles(int N, const char* dirname){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int writeFile(const char* pathname, const char* dirname){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);
/*
 *
 *
 *
 *
 * */

int lockFile(const char* pathname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

int unlockFile(const char* pathname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

int closeFile(const char* pathname){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int removeFile(const char* pathname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

