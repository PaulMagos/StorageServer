/*
 Created by paul on 24/01/22.
*/

#ifndef STORAGESERVER_SERVER_H
#define STORAGESERVER_SERVER_H

#include "list.h"
#include "conn.h"
#include "log.h"
#include "icl_hash.h"


/* -------------------------------- SERVER STATUS -------------------------------- */
typedef enum{
    E,                                      /* Enabled */
    Q,                                      /* Quitting, serve last requests only */
    S                                       /* Turned Off */
} serverStat;

/* Policy for replacement */
typedef enum cPolicy {
    FIFO,                                   /* Firs In, First Out */
    LIFO,                                   /* Last In First Out */
    LRU,                                    /* Last recently used */
    MRU                                     /* Most recently used */
} cachePolicy;


/* -------------------------------- SigHandler Args -------------------------------- */

typedef struct{
    int pipe;                               /* Pipe for communication between threads */
    sigset_t *sigSet;                       /* Signal set for the signal handler */
} sigHandlerArgs;

/* -------------------------------- Clients Struct -------------------------------- */

typedef struct{
    List ClientsFds;                        /* Client List */
    pthread_mutex_t lock;                   /* Client list lock */
} clientList;

/* -------------------------------- SERVER CONFIG -------------------------------- */
typedef struct{
    int maxFile;                            /* Server max number of files */
    size_t maxByte;                         /* Capacity of the server */
    int threadNumber;                       /* Number of threads that are working */
    cachePolicy policy;                     /* File expulsion policy */
    char serverSocketName[UNIX_PATH_MAX];   /* Socket name for listening */
} serverConfig;

/* -------------------------------- FILES STRUCT -------------------------------- */
typedef struct {
    /* File itself */
    char* path;                             /* The path in which hypothetically the server will store the files */
    size_t size;                            /* The size of the file in bytes */
    void* content;                          /* The actual content of the file */

    /* File accessing info */
    int lockFd;                             /* Fd of the client who locked the file */
    int writers;                            /* Number of writers at a given moment */
    int readers;                            /* Number of readers at a given moment */
    int toDelete;                           /* 1 if toDelete, 0 if not */
    List clients_fd;                        /* Fd of the clients who have opened the file */
    fileFlags latsOp;                       /* Last Operation */
    struct timespec lastOpTime;             /* Last operation time */
    struct timespec creationTime;           /* Creation time of the file */


    /* Mutex variables */
    pthread_mutex_t lock;                   /* Mutex variable for lock */
    pthread_mutex_t order;                  /* Access order Mutex */
    pthread_cond_t condition;               /* Condition variable for access blocking */
} serverFile;

/* --------------------------------------- Server Struct --------------------------------------- */
typedef struct{
    int stdOutput;                          /* Bool variable to see operations on the terminal */
    int connected;                          /* The number of clients connected now */
    int deletedFiles;                       /* Number of deleted files */
    serverStat status;                      /* Server status */
    int expelledFiles;                      /* Number of expelled files */
    int maxConnections;                     /* The max number of client connected in one session */
    size_t deletedBytes;                    /* Number of deleted files bytes */
    size_t expelledBytes;                   /* Number of expelled bytes */
    pthread_mutex_t lock;                   /* Mutex variable for lock */
    int actualFilesNumber;                  /* Number of files in one moment of the working session */
    size_t sessionMaxBytes;                 /* Max bytes of file in one working session */
    size_t actualFilesBytes;                /* Num of bytes in a momento of the working session */
    icl_hash_t * filesTable;                /* Server storage hash table */
    int sessionMaxFilesNumber;              /* Max files in one working session */
    char serverLog[UNIX_PATH_MAX];          /* Log path for server */
} fileServer;

/* ------------------------------------ Thread Pool Struct ------------------------------------ */
typedef struct task_{
    void (*func)(void*);                    /* Generic function with void argument */
    void* arguments;                        /* Void argument of many kinds */
    struct task_ *next;                     /* Pointer to the next task */
} queueTask;

typedef struct{
    int stop;                               /* Serve the last clients and close the thread pool or literally stop */
    int taskN;                              /* Queue Len */
    int maxTasks;                           /* Max number of task executed */
    int maxThreads;                         /* Max number of threads allowed */
    int taskRunning;                        /* Current number of running tasks */
    int** workersPipes;                     /* ThreadPool thread's pipes for communication with main thread */
    queueTask *taskHead;                    /* Head of tasks list */
    queueTask *taskTail;                    /* Tail of tasks list */
    pthread_t* workers;                     /* Workers ids */
    pthread_mutex_t lock;                   /* Lock variable for mutex access */
    pthread_cond_t work_cond;               /* Signals to the threads that there is work to be done */
} threadPool;

/* -------------------------------- Worker arguments -------------------------------- */
typedef struct {
    int pipeT;
    int worker_id;
    int client_fd;
} wTask;

extern serverConfig ServerConfig;
extern fileServer* ServerStorage;
extern logFile ServerLog;
extern clientList* fd_clients;

/* ------------------------------------ Thread Pool Functions ------------------------------------ */


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


/* ------------------------------------ Worker Functions ------------------------------------ */
/**
 *   @func  taskExecute
 *   @effects executes the requests from the clients
 *   @param argument -> wTask variable with client fd, worker id and pipe for communications
 */
void taskExecute (void* argument);

/**
 *   @func  CloseConnection
 *   @effects closes the connection to the client on fd_client, if needed unlocks all the files locked by fd_client
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
 *   @return returns 0 in case everything goes right, -1 if an error occurs
*/
int CloseConnection(int fd_client, int workerId);
/**
 *   @func  OpenFile
 *   @effects read from the fd_client the path and the flags for opening a file
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void OpenFile(int fd_client, int workerId, message* message1);
/**
 *   @func  CloseFile
 *   @effects close a file previously opened by 'fd' client
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void CloseFile(int fd_client, int workerId, message* );
/**
 *   @func  DeleteFile
 *   @effects delete a file
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void DeleteFile(int fd_client, int workerId, message* );
/**
 *   @func  ReceiveFile
 *   @effects write a file from the client 'fd' in the server
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void ReceiveFile(int fd_client, int workerId, message* );
/**
 *   @func  SendFile
 *   @effects send a file to the client 'fd' from the server
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void SendFile(int fd_client, int workerId, message* );
/**
 *   @func  SendNFile
 *   @effects send N files to the client 'fd' from the server
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void SendNFiles(int fd_client, int workerId, message* );
/**
 *   @func  SendNFile
 *   @effects lock a file for the client 'fd' in the server
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void LockFile(int fd_client, int workerId, message* );
/**
 *   @func  SendNFile
 *   @effects unlock a file for the client 'fd' in the server
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void UnLockFile(int fd_client, int workerId, message* );
/**
 *   @func  AppendOnFile
 *   @effects append on a file that exists from the client 'fd' in the server
 *   @param fd_client -> client_fd
 *   @param workerId-> thread id
*/
void AppendOnFile(int fd_client, int workerId, message* );
/* ------------------------------------ Files Functions ------------------------------------ */
/**
 *   @func  fileReadersIncrement
 *   @effects increments the readers counter
 *   @param file -> file for the operation
 *   @param thread -> thread id
 */
int fileReadersIncrement(serverFile* file, int thread);
/**
 *   @func  fileReadersDecrement
 *   @effects decrements the readers counter
 *   @param file -> file for the operation
 *   @param thread -> thread id
 */
int fileReadersDecrement(serverFile* file, int thread);
/**
 *   @func  fileWritersIncrement
 *   @effects increments the writers counter
 *   @param file -> file for the operation
 *   @param thread -> thread id
 */
int fileWritersIncrement(serverFile* file, int thread);
/**
 *   @func  fileWritersDecrement
 *   @effects decrements the writers counter
 *   @param file -> file for the operation
 *   @param thread -> thread id
 */
int fileWritersDecrement(serverFile* file, int thread);
/**
 *   @func  isLocked
 *   @param file -> file for the operation
 *   @param fd -> file descriptor to test
 *   @return 0 if fd locked the file previously, 1 if is locked by another client, -1 if not locked
 */
int isLocked(serverFile* file, long fd);

/**
 * @func  Returns the file that can be replaced between file1 and file2
 * @param file1 -- the file to which compare the time of creation or last operations
 * @param file2 -- the second file to which compare the time of creation or last operations
 * @param policy -- the policy to be used for the comparison
 * @returns file1 or file2 to replace on success, NULL on failure.
 */
serverFile* replaceFile(serverFile* file1, serverFile* file2);

/**
 * @func  Returns a replaceable file
 * @param ht -- the icl_hash table of files
 * @param policy -- the policy to be used for the comparison
 * @param workerId -- the thread id
 * @returns file to replace on success, NULL on failure.
 */
serverFile
* icl_hash_toReplace(icl_hash_t *ht, int workerId);

/**
 * @func  Returns a string with the policy in "policy"
 * @param policy -- the policy
 * @returns policy in string on success, NULL on failure.
 */
char* fromEnumCachePolicyToString(cachePolicy policy);

/**
 * @func  Returns a enum cachePolicy with the policy in "str"
 * @param str -- the string with policy
 * @returns policy in cachePolicy on success, FIFO on failure.
 */
cachePolicy fromStringToEnumCachePolicy(char* str);

/**
 * freeFile
 * @func Frees the memory allocated for the file
 * @param file -- the file to free
 */
void freeFile(void* file);

/**
 * icl_hash_toDelete
 * @func finds files to delete, that has to be sent to fd
 * @param ht -- the files table
 * @param expelled -- the list in which to put the files
 * @param fd -- the client
 * @param workerId -- the thread id
 * @return 0 if none, num of files if any
 */
int icl_hash_toDelete(icl_hash_t * ht, List expelled, int fd, int workerId);
/* ------------------------------------ Server Functions ------------------------------------ */
/** defaultConfig
 * @func  Sets the Server config with default settings
 * @param config Config structure to set
 */
 void defaultConfig(serverConfig* config);

/** fileNumIncrement
 * @func  Increments the Server momentum number of files
 */
void fileNumIncrement();
/** fileNumDecrement
 * @func  Decrements the Server momentum number of files
 */
void fileNumDecrement();
/** fileBytesIncrement
 * @func  Increments the Server momentum bytes
 * @param size increment size
 */
void fileBytesIncrement(size_t size);
/** fileBytesDecrement
 * @func  Decrements the Server momentum bytes
 * @param size decrement size
 */
void fileBytesDecrement(size_t size);

#endif /*STORAGESERVER_SERVER_H */
