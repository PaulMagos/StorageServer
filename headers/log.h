/*
 Created by paul on 26/01/22.
*/

#ifndef STORAGESERVER_LOG_H
#define STORAGESERVER_LOG_H

#include "utils.h"

/* -------------------------------- LogFile Info -------------------------------- */
typedef struct{
    FILE* file;                             /* Log File pointer */
    pthread_mutex_t mutex;                  /* Mutex for mutual access */
} logF;

typedef logF* logFile;

/* From : https://www.techiedelight.com/print-current-date-and-time-in-c/ */
static inline void getTime(char** val, int type){
    time_t now;
    struct tm *local;
    *val = malloc(20*sizeof(char));
    if(!(*val)) return;
    /* `time_t` is an arithmetic time type */

    /*
     * Obtain current time
     * time()` returns the current time of the system as a `time_t` value
     */
    time(&now);

    /*
     * localtime converts a `time_t` value to calendar time and
     * returns a pointer to a `tm` structure with its members
     * filled with the corresponding values
     */
    local = localtime(&now);

    if(type == 0)
    snprintf(*val, 20, "%d-%d-%d_%d:%d:%d", local->tm_mday, local->tm_mon+1, local->tm_year+1900,
             local->tm_hour, local->tm_min, local->tm_sec);
    else
        snprintf(*val, 20, "%d:%d:%d", local->tm_hour, local->tm_min, local->tm_sec);
}

/**
 *   @func  CreateLog
 *   @param dir -> log files directory
 *   @param log -> initialize logF* structure with file and mutex
 *   @effects returns nothing, create a file in the directory pointed by dir, if exists, or creates it if not.
 */
void createLog(char* dir, logFile* log);

/**
 *   @func  appendOnLog
 *   @param log -> logF* structure with file and mutex
 *   @param strBuf -> what we have to append on the log
 *   @param ... -> va_args associated with the operation described in strBuf
 *   @effects -> write on the log file the buffer and the hypothetical other messages
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int appendOnLog(logFile log, char* strBuf,...);

/**
 *   @func -> closeLogStr
 *   @param log -> logF* structure with file and mutex, we have to free this structure
 *   @effects -> it frees the log structure
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int closeLogStr(logFile log);

/**
 *   @func -> logSeparator
 *   @param log -> logF* structure with file and mutex, we have to free this structure
 *   @effects -> prints a line separator on the log
 */
void logSeparator(logFile log);

#endif /* STORAGESERVER_LOG_H */
