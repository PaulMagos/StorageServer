//
// Created by paul on 26/01/22.
//

#include "../../../headers/server.h"

static queueTask* getTask(threadPool* tPool){
    queueTask* tmpTask;

    tmpTask = tPool->taskHead;
    if(tmpTask == NULL) return NULL;

    if(tmpTask->next == NULL){
        tPool->taskHead = NULL;
        tPool->taskTail = NULL;
    } else{
        tPool->taskHead = tmpTask->next;
    }

    return tmpTask;
}

static queueTask* threadPoolTaskCreate(void (*func)(void*), void* arguments){
    queueTask *task;

    task = malloc(sizeof(*task));
    task->func = func;
    task->arguments = arguments;
    task->next = NULL;
    return task;
}

static void threadPoolTaskDestroy(queueTask* task){
    free(task);
}

static void *threadPoolWorker(void* arg){
    threadPool* tmpPool  = arg;
    queueTask* tmpTask;
    pthread_t self = pthread_self();
    int id;
    for(int i = 0; i < tmpPool->threadCnt; i++){
        if(pthread_equal(self, tmpPool->workers[i])) {
            id = i;
            break;
        }
    }
    appendOnLog(ServerLog, "Thread %d: Started\n", id);

    while (1){
        pthread_mutex_lock(&(tmpPool->lock));
        while ( tmpPool->taskHead == NULL && tmpPool->stop==0){
            pthread_cond_wait(&(tmpPool->work_cond), &(tmpPool->lock));
        }

        if(tmpPool->stop>1) break;

        if(tmpPool->stop==1 && tmpPool->taskN == 0) break;


        tmpTask = getTask(tmpPool);
        tmpPool->taskN--;
        pthread_mutex_unlock(&(tmpPool->lock));


        if(tmpTask != NULL){
            tmpTask->func(tmpTask->arguments);
            threadPoolTaskDestroy(tmpTask);
        }
    }
    appendOnLog(ServerLog, "Thread %d: Stopped\n", id);
    pthread_mutex_unlock(&(tmpPool->lock));
    pthread_exit(NULL);
}

threadPool* initThreadPool(int threads){
    if(threads <= 0) {
        errno = EINVAL;
        return NULL;
    }

    threadPool* threadPool1 = (threadPool*)calloc(1, sizeof (threadPool));
    if ( threadPool1 == NULL ) {
        return NULL;
    }


    threadPool1->stop = 0;
    threadPool1->taskN = 0;
    threadPool1->maxTasks = 0;
    threadPool1->taskRunning = 0;
    threadPool1->maxThreads = threads;
    threadPool1->workers = (pthread_t*)calloc(threads, sizeof(pthread_t));
    if(threadPool1->workers == NULL){
        free(threadPool1);
        return NULL;
    }
    if((pthread_mutex_init(&(threadPool1->lock), NULL) != 0) ||
       (pthread_cond_init(&(threadPool1->work_cond), NULL) != 0) ){
        stopThreadPool(threadPool1,true);
        return NULL;
    }

    for (int i = 0; i < threadPool1->maxThreads; i++){
        if(pthread_create(&(threadPool1->workers[i]), NULL, threadPoolWorker, (void*)threadPool1) != 0){
            stopThreadPool(threadPool1, 1);
            errno = EFAULT;
            return NULL;
        }
        threadPool1->threadCnt++;
    }
    appendOnLog(ServerLog, "ThreadPool: Started\n");
    return threadPool1;
}

void freePool(threadPool* tPool){
    if(tPool->workers != NULL){
        free(tPool->workers);

        pthread_mutex_destroy(&(tPool->lock));
        pthread_cond_destroy(&(tPool->work_cond));
    }
    free(tPool);
    return;
}

int stopThreadPool(threadPool* tPool, int hard_off){
    if(tPool == NULL){
        errno = EINVAL;
        return -1;
    }

    SYSCALL_RETURN(stopThreadPool_mutex_lock, pthread_mutex_lock(&(tPool->lock)),
                   "ERROR : pthread_mutex_lock failed, errno = %d", errno);

    tPool->stop = 1 + ((hard_off)? 1:0);

    if(pthread_cond_broadcast(&(tPool->work_cond)) != 0){
        SYSCALL_RETURN(stopThreadPool_mutex_unlock, pthread_mutex_unlock(&(tPool->lock)),
                       "ERROR : pthread_mutex_unlock failed, errno = %d", errno);
        errno = EFAULT;
        return -1;
    }

    SYSCALL_RETURN(stopThreadPool_mutex_unlock, pthread_mutex_unlock(&(tPool->lock)),
                   "ERROR : pthread_mutex_unlock failed, errno = %d", errno);

    while(tPool->taskN>0){
        queueTask* tmpTask = getTask(tPool);
        free(tmpTask);
        tPool->taskN--;
    }

    for(int i = 0; i < tPool->threadCnt; i++){
        if(pthread_join(tPool->workers[i], NULL) != 0){
            errno = EFAULT;
            return -1;
        }
    }

    freePool(tPool);

    appendOnLog(ServerLog, "ThreadPool: Stopped\n");
    return 0;
}

int enqueue(threadPool* tPool, void (*func)(void*), void* arguments){
    if(tPool==NULL || func == NULL){
        errno = EINVAL;
        return -1;
    }

    SYSCALL_RETURN(enqueue_mutex_lock, pthread_mutex_lock(&(tPool->lock)),
                   "ERROR : pthread_mutex_lock failed, errno = %d", errno);



    int res = 0;

    queueTask* tmpTask;
    tmpTask = threadPoolTaskCreate(func, arguments);
    if(tPool->taskHead == NULL){
        tPool->taskHead = tmpTask;
        tPool->taskTail = tPool->taskHead;
    } else{
        tPool->taskTail->next = tmpTask;
        tPool->taskTail = tmpTask;
    }

    tPool->taskN++;

    SYSCALL_EXIT(enqueue_pthread_cond_broadcast, res, pthread_cond_broadcast(&(tPool->work_cond)),
                 "ERROR : pthread_cond_broadcast failed, errno = %d", errno);
    SYSCALL_RETURN(enqueue_mutex_unlock, pthread_mutex_unlock(&(tPool->lock)),
                   "ERROR : pthread_mutex_unlock failed, errno = %d", errno);
    if(res != 0){
        errno = res;
        return -1;
    }
    return 0;
}
