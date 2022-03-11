//
// Created by paul on 24/01/22.
//

#ifndef STORAGESERVER_SERVER_H
#define STORAGESERVER_SERVER_H
#define DEFAULT_CONFIG {100, 15728640, 15, 0, "../../tmp/cs_socket"}

#include "list.h"
#include "conn.h"
#include "log.h"
#include "icl_hash.h"


// -------------------------------- SERVER STATUS --------------------------------
typedef enum{
    E = 0,                                  // Enabled
    Q = 1,                                  // Quitting, serve last requests only
    S = -1,                                 // Turned Off
} serverStat;

// Policy for replacement
typedef enum {
    FIFO = 0,                                   // Firs In, First Out
    LIFO = 1,                                   // Last In First Out
    LRU = 2,                                    // Last recently used
    MRU = 3,                                    // Most recently used
} cachePolicy;


// -------------------------------- SigHandler Args --------------------------------

typedef struct{
    int pipe;                               // Pipe for communication between threads
    sigset_t *sigSet;                       // Signal set for the signal handler
} sigHandlerArgs;

// -------------------------------- Clients Struct --------------------------------

typedef struct{
    List ClientsFds;                        // Client List
    pthread_mutex_t lock;                   // Client list lock
} clientList;

// -------------------------------- SERVER CONFIG --------------------------------
typedef struct{
    int maxFile;                            // Server max number of files
    size_t maxByte;                         // Capacity of the server
    int threadNumber;                       // Number of threads that are working
    cachePolicy policy;                     // File expulsion policy
    char serverSocketName[UNIX_PATH_MAX];   // Socket name for listening
} serverConfig;

// -------------------------------- FILES STRUCT --------------------------------
typedef struct {
    // File itself
    char* path;                             // The path in which hypothetically the server will store the files
    size_t size;                            // The size of the file in bytes
    void* content;                          // The actual content of the file

    // File accessing info
    int writers;                            // Number of writers at a given moment
    int readers;                            // Number of readers at a given moment
    int* client_fds;                        // List of all clients who have accessed this file
    int clients;                            // Number of client who have accessed this file
    int deletable;                          // 1 if deletable, 0 if not
    operation latsOp;                       // Last Operation
    struct timespec creationTime;           // Creation time of the file
    struct timespec lastOpTime;             // Last operation time


    // Mutex variables
    pthread_mutex_t lock;                   // Mutex variable for lock
    pthread_mutex_t order;                  // Access order Mutex
    pthread_cond_t condition;               // Condition variable for access blocking
} serverFile;

 // -------------------------------- WORKER WORKING STRUCT --------------------------------
typedef struct{
    int stdOutput;                          // Bool variable to see operations on the terminal
    serverStat status;                      // Server status
    int expelledFiles;                      // Number of expelled files
    int maxConnections;                     // The max number of client connected in one session
    int actualFilesNumber;                  // Number of files in one moment of the working session
    pthread_mutex_t lock;                   // Mutex variable for lock
    size_t sessionMaxBytes;                 // Max bytes of file in one working session
    size_t actualFilesBytes;                // Num of bytes in a momento of the working session
    icl_hash_t * filesTable;                // Server storage hash table
    int sessionMaxFilesNumber;              // Max files in one working session
    char serverLog[UNIX_PATH_MAX];          // Log path for server
} fileServer;

// ------------------------------------ Thread Pool Struct ------------------------------------
typedef struct task_{
    void (*func)(void*);                // Generic function with void argument
    void* arguments;                    // Void argument of many kinds
    struct task_ *next;                 // Pointer to the next task
} queueTask;

typedef struct{
    int stop;                           // We have to serve the last clients and close the thread pool or literally stop
    int taskN;                          // Queue Len
    int maxTasks;                       // Max number of task executed
    int threadCnt;                      // Thread counter
    int maxThreads;                     // Max number of threads allowed
    int taskRunning;                    // Current number of running tasks
    queueTask *taskHead;                // Head of tasks list
    queueTask *taskTail;                // Tail of tasks list
    pthread_t* workers;                 // Workers ids
    pthread_mutex_t lock;               // Lock variable for mutex access
    pthread_cond_t work_cond;           // Signals to the threads that there is work to be done
} threadPool;

// -------------------------------- Worker arguments --------------------------------
typedef struct {
    int pipe;
    int client_fd;
    int worker_id;
} wTask;


extern serverConfig ServerConfig;
extern fileServer* ServerStorage;
extern logFile ServerLog;
extern clientList* fd_clients;

// ------------------------------------ Thread Pool Functions ------------------------------------


/**
 *   @func  initThreadPool
 *   @effects creates a threadPool structure and returns it
 *   @param threads -> number of workers of the thread-pool
 *   @return returns the initialized thread pool, NULL in case an error occurs
 */
threadPool* initThreadPool(int threads);

/**
 *   @func  enqueue
 *   @effects adds the task to the pending queue or executes it immediately by assigning a thread if there is one free
 *   @param tPool -> Thread-pool structure
 *   @param func,arguments -> Task to enqueue, with fun and argument
 *   @return returns 0 in case everything goes right, 1 if there is no space in the queue or there are no threads that
 *              can handle the task at the moment, -1 if an error occurs
 */
int enqueue(threadPool* tPool, void (*func)(void*), void* arguments);

/**
 *   @func  closeThreadPool
 *   @effects if hard_off is true it closes immediately all the threads and frees the space
 *   @param tPool -> Thread-pool structure
 *   @param hard_off -> if 1 it forces all the thread to go down, if 0 the thread will work on the pending
 *                         task then close
 *   @return returns 0 in case everything goes right, -1 if an error occurs
 */
int stopThreadPool(threadPool* tPool, int hard_off);


// ------------------------------------ Worker Functions ------------------------------------
/**
 *   @func  taskExecute
 *   @effects executes the requests from the clients
 *   @param argument -> wTask variable with client fd, worker id and pipe for communications
 */
void taskExecute (void* argument);

// ------------------------------------ Worker Functions ------------------------------------
/**
 *   @func  lockFile
 *   @effects locks the file
 *   @param file -> file for the lock operation
 *   @param mode -> lock mode, 0 read mode, 1 write mode
 *   @param locker -> if not -1, is the client fd that wants the lock
 */
int lockFile(serverFile* file, int mode, int locker);

/**
 *   @func  unlockFile
 *   @effects unlocks the file
 *   @param file -> file for the lock operation
 *   @param mode -> previous lock mode, 0 read mode, 1 write mode
 *   @param locker -> if not -1, is the client fd that have locked the file before
 */
int unlockFile(serverFile* file, int mode, int locker);

/**
 *   @func  newAccessFrom
 *   @effects add the fd to the file openers fds
 *   @param file -> file for operation
 *   @param clientFd -> is the client fd that wants to access the file
 */
int newAccessFrom(serverFile* file, int clientFd);

/**
 * @func  Returns a replaceable file
 * @param file1 -- the file to which compare the time of creation or last operations
 * @param file2 -- the second file to which compare the time of creation or last operations
 * @param policy -- the policy to be used for the comparison
 * @returns file to replace on success, NULL on failure.
 */
static serverFile* replaceFile(serverFile* file1, serverFile* file2, cachePolicy policy){
    if(file1 == NULL && file2 == NULL) return NULL;
    if(file1 == NULL) return file2;
    if(file2 == NULL) return file1;
    switch (policy) {
        case FIFO: return tSpecCmp(file1->creationTime, file2->creationTime,>)? file2 : file1;
        case LIFO: return tSpecCmp(file1->creationTime, file2->creationTime,>)? file1 : file2;
        case LRU:  return tSpecCmp(file1->lastOpTime, file2->lastOpTime,>)? file2 : file1;
        case MRU:  return tSpecCmp(file1->lastOpTime, file2->lastOpTime,>)? file1 : file2;
        default: return NULL;
    }
    return NULL;
}


#endif //STORAGESERVER_SERVER_H
