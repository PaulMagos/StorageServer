//
// Created by paul on 16/01/22.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "errno.h"

#include "../../headers/api.h"
#include "../../headers/utils.h"


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

int closeConnection(const char* sockname);
/*
 *
 *
 *
 *
 * */

int openFile(const char* pathname, int flags);
/*
 *
 *
 *
 *
 * */

int readFile(const char* pathname, void** buf, size_t* size);
/*
 *
 *
 *
 *
 * */

int readNFiles(int N, const char* dirname);
/*
 *
 *
 *
 *
 * */

int writeFile(const char* pathname, const char* dirname);
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

int lockFile(const char* pathname);
/*
 *
 *
 *
 *
 * */

int unlockFile(const char* pathname);
/*
 *
 *
 *
 *
 * */

int closeFile(const char* pathname);
/*
 *
 *
 *
 *
 * */

int removeFile(const char* pathname);
/*
 *
 *
 *
 *
 * */

