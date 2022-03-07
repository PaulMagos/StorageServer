//
// Created by paul on 24/01/22.
//

#ifndef STORAGESERVER_SERVER_H
#define STORAGESERVER_SERVER_H
#define DEFAULT_CONFIG {100, 15728640, 15, E, FIFO, "../../tmp/cs_socket"}

#include "utils.h"
#include "list.h"
#include "conn.h"
#include "log.h"

 // -------------------------------- SERVER STATUS --------------------------------
typedef enum{
    E,                                      // Enabled
    Q,                                      // Quitting, serve last requests only
    S,                                      // Turned Off
} serverStat;

// Policy for replacement
typedef enum {
    FIFO,                                   // Firs In, First Out
    LIFO,                                   // Last In First Out
    LRU,                                    // Last recently used
    MRU,                                    // Most recently used
} cachePolicy;

// Struct for server configuration
typedef struct{
    int maxFile;                            // Server max number of files
    size_t maxByte;                         // Capacity of the server
    int threadNumber;                       // Number of threads that are working
    serverStat status;                      // Server status
    cachePolicy policy;                     // File expulsion policy
    char serverSocketName[UNIX_PATH_MAX];   // Socket name for listening
} serverConfig;

 // -------------------------------- WORKER WORKING STRUCT --------------------------------
typedef struct{
    bool stdOutput;                         // Bool variable to see operations on the terminal
    int expelledFiles;                      // Number of expelled files
    int maxConnections;                     // The max number of client connected in one session
    int actualFilesNumber;                  // Number of files in one moment of the working session
    pthread_mutex_t mutex;                  // Mutex variable to access
    size_t sessionMaxBytes;                 // Max bytes of file in one working session
    size_t actualFilesBytes;                // Num of bytes in a momento of the working session
    int sessionMaxFilesNumber;              // Max files in one working session
    char serverLog[UNIX_PATH_MAX];          // Log path for server
} fileServerStat;

 // -------------------------------- FILES STRUCT --------------------------------
typedef struct {
    // File itself
    char* path;                             // The path in which hypothetically the server will store the files
    size_t size;                            // The size of the file in bytes
    void* content;                          // The actual content of the file

    // File accessing info
    int writers;                            // Number of writers at a given moment
    int readers;                            // Number of readers at a given moment
    List client_fds;                        // List of all clients who have accessed this file
    struct timespec creationTime;           // Creation time of the file
    struct timespec lastOpTime;             // Last operation time
    operation latsOp;                       // Last Operation

    // Mutex variables
    pthread_mutex_t mutex;                  // Mutex
    pthread_mutex_t order;                  // Access order Mutex
    pthread_cond_t condition;               // Condition variable for working threads
} serverFile;

 // -------------------------------- GLOBAR VARIABLES --------------------------------
serverConfig config;
fileServerStat* fileServer;
logFile serverLog;




#endif //STORAGESERVER_SERVER_H
