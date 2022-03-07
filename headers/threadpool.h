//
// Created by paul on 26/01/22.
//

#ifndef STORAGESERVER_THREADPOOL_H
#define STORAGESERVER_THREADPOOL_H

#define MAX_THREADS 20
#define MAX_TASKS 5000

#include "list.h"
#include "utils.h"


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

// ------------------------------------ Thread Pool Functions ------------------------------------


/*
 *   @func  initThreadPool
 *   @effects creates a threadPool structure and returns it
 *   @param threads -> number of workers of the thread-pool
 *   @return returns the initialized thread pool, NULL in case an error occurs
 */
threadPool* initThreadPool(int threads);

/*
 *   @func  enqueue
 *   @effects adds the task to the pending queue or executes it immediately by assigning a thread if there is one free
 *   @param tPool -> Thread-pool structure
 *   @param func,arguments -> Task to enqueue, with fun and argument
 *   @return returns 0 in case everything goes right, 1 if there is no space in the queue or there are no threads that
 *              can handle the task at the moment, -1 if an error occurs
 */
int enqueue(threadPool* tPool, void (*func)(void*), void* arguments);

/*
 *   @func  closeThreadPool
 *   @effects if hard_off is true it closes immediately all the threads and frees the space
 *   @param tPool -> Thread-pool structure
 *   @param hard_off -> if true it forces all the thread to go down, if false the thread will work on the pending
 *                         task then close
 *   @return returns 0 in case everything goes right, -1 if an error occurs
 */
int stopThreadPool(threadPool* tPool, bool hard_off);




#endif //STORAGESERVER_THREADPOOL_H
