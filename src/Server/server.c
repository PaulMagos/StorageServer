/*
 * Created by paul magos on 11/01/22.
 */

#include "../../headers/server.h"

/* -------------------------------- GLOBAR VARIABLES -------------------------------- */
serverConfig ServerConfig;
fileServer* ServerStorage;
logFile ServerLog;
clientList* fd_clients;
pthread_t sigHandler_t;
threadPool* threadPool1;

int* sigHandler_Pipe;

void serverDestroy();
void newClient();
void removeClient();
static void* signalHandler(void* arguments);
int signalHandlerDestroy();
int serverInit(char* configPath, char* logPath);
void printServerStatus();

int main(int argc, char* argv[]){
    /* SYSCALL RESPONSE VARIABLE */
    int res = 0, threadId, i, j, c, max_fd = 0, fd_server_socket, fd_client_socket = 0, cont;
    struct sockaddr_un SocketAddress;
    /* SigPipe */
    struct sigaction act;
    /* int* sigHandler_Pipe; */
    sigset_t mask;
    sigHandlerArgs sigArgs;
    /* CLIENTS FDS */
    fd_set readySet, currSet;
    /* -------------------------------------    Server   -------------------------------------
     * Log, Config and Server Init
     */
    if(argc == 1){
        serverInit(NULL, NULL);
        fprintf(stdout, "No arguments, Server will start with default parameters\n");
    }else if(argc == 2){
        serverInit(argv[1], NULL);
    }else if(argc == 3){

        serverInit(argv[1], argv[2]);
    }else if(argc>2){
        fprintf(stderr, "ERROR - Too many argument, max 2 arguments: \n");
        printf("Example: ./server config_path log_directory\n");
        exit(0);
    }


    atexit(serverDestroy);


    /*
     * ------------------------------------- SigHandler -------------------------------------
     * Signal Handler & Respective Pipe Init
     * Signal Set creation for the sigHandler thread
     */
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    SYSCALL_RETURN(pthread_sigmask, pthread_sigmask(SIG_BLOCK, &mask, NULL),
                   "ERROR - pthread_sigmask fault, errno = %d\n", errno)

    /* SigPipe Ignore */
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    SYSCALL_RETURN(sigaction, sigaction(SIGPIPE, &act, NULL),
                   "ERROR - sigaction failure to ignore sigpipe, errno = %d\n", errno)


    /* Pipe init for threads communications */
    sigHandler_Pipe = malloc(2*sizeof(int));
    SYSCALL_RETURN(pipe, pipe(sigHandler_Pipe),
                   "ERROR - pipe failure, errno = %d", errno);

    /* pthread_t sigHandler_t; */
    sigArgs.pipe = sigHandler_Pipe[1];
    sigArgs.sigSet = &mask;
    SYSCALL_RETURN(sigHandler_pthread_creation,
                   pthread_create(
                           &sigHandler_t,
                           NULL,
                           signalHandler,
                           &sigArgs
                   ),
                   "ERROR - Signal Handler Thread Creation Failure, errno = %d", errno);
    /*
     * ------------------------------------- ThreadPool -------------------------------------
     * ThreadPool Initialize
     */
    threadPool1 = initThreadPool(ServerConfig.threadNumber);
    if(!threadPool1){
        //free(sigHandler_Pipe);
        fprintf(stderr, "ERROR - threadPool init failure");
        exit(EXIT_FAILURE);
    }

    /*
     * ------------------------------------- ServerSocket -------------------------------------
     * Socket creation
     */
    max_fd = 0;
    fd_client_socket = 0;
    (void) unlink(ServerConfig.serverSocketName);
    memset(&SocketAddress, '0', sizeof(SocketAddress));
    strncpy(SocketAddress.sun_path, ServerConfig.serverSocketName, UNIX_PATH_MAX);
    SocketAddress.sun_family = AF_UNIX;

    /* Socket binding and ready to listen */
    fd_server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd_server_socket == -1) {
        //free(sigHandler_Pipe);
        fprintf(stderr,  "ERROR - Socket set failure, errno = %d\n", errno);
        exit(errno);
    }
    res = bind(fd_server_socket, (struct sockaddr*) &SocketAddress, sizeof(SocketAddress));
    if(res==-1) {
        fprintf(stderr,  "ERROR - Socket %s bind failure, errno = %d\n", ServerConfig.serverSocketName, errno);
        exit(errno);
    }
    res = listen(fd_server_socket, SOMAXCONN);
    if(res==-1) {
        //free(sigHandler_Pipe);
        fprintf(stderr,  "ERROR - Socket listen failure, errno = %d\n", errno);
        exit(errno);
    }

    /* ------------------------------------- Clients Fds ------------------------------------- */
    FD_ZERO(&currSet);
    FD_ZERO(&readySet);
    FD_SET(fd_server_socket, &currSet);
    FD_SET(sigHandler_Pipe[0], &currSet);
    if(fd_server_socket>max_fd) max_fd = fd_server_socket;
    for(i = 0; i<ServerConfig.threadNumber; i++){
        FD_SET(threadPool1->workersPipes[i][0], &currSet);
        if(i > max_fd) max_fd = i;
    }



    if(ServerStorage->stdOutput) fprintf(stdout, "Server Started Well, Waiting For Connections\n");
    /* ----------------------------------- MainThreadFunc ------------------------------------ */
    while (ServerStorage->status == E){
        readySet = currSet;
        res = select(max_fd+1, &readySet,NULL, NULL, NULL);
        if(res==-1) {
            fprintf(stderr, "ERROR - Select failed, errno = %d", errno);
            exit(errno);
        }
        for(i = 0; i <= max_fd; i++){
            if(!FD_ISSET(i, &readySet)) continue;
            if(i==fd_server_socket && ServerStorage->status==E && max_fd<1000){
                fd_client_socket = accept(fd_server_socket, NULL, NULL);
                if(fd_client_socket==-1){
                    if(ServerStorage->stdOutput) fprintf(stderr, "ERROR - Accept new connection to server, errno = %d", errno);
                    continue;
                }
                newClient();
                FD_SET(fd_client_socket, &currSet);
                if(fd_client_socket > max_fd) max_fd = fd_client_socket;
                continue;
            }

            if(i == sigHandler_Pipe[0]){
                if(ServerStorage->status==S){
                    for(c = 0; c<=max_fd; c++){
                        if(FD_ISSET(c, &currSet)) close(c);
                    }
                    break;
                } else{
                    continue;
                }
            }

            cont = 1;
            for(threadId = 0; threadId < ServerConfig.threadNumber; threadId++){
                if(i == threadPool1->workersPipes[threadId][0]){
                    if(readn(threadPool1->workersPipes[threadId][0], &fd_client_socket, sizeof(int)) == -1){
                        fprintf(stderr, "ERROR - Reading Thread %d Pipe", threadId);
                        continue;
                    }
                    newClient();
                    FD_SET(fd_client_socket, &currSet);
                    if(fd_client_socket > max_fd) max_fd = fd_client_socket;
                    cont = 0;
                }
            }
            if(!cont) continue;

            enqueue(threadPool1, taskExecute, &i);

            FD_CLR(i, &currSet);
            removeClient();
            if(i == max_fd){
                for(j = max_fd; j>=0; j--){
                    if(FD_ISSET(j, &currSet)){
                        max_fd = j;
                        break;
                    }
                }
            }
        }
    }


    msleep(2000);
    /* serverDestroy(); */
    //free(sigHandler_Pipe);
    if(res!=0) res = 0;
    return res;
}

static void* signalHandler(void *arguments){
    sigset_t *workingSet = ((sigHandlerArgs*)arguments)->sigSet;
    int pipeFd = ((sigHandlerArgs*)arguments)->pipe;
    int signal;
    int res;
    appendOnLog(ServerLog, "[SignalHandler]: Started\n");
    while(1){
        res = sigwait(workingSet, &signal);
        if(res != 0){
            fprintf(stderr, "ERROR - sigWait failure, errno = %d\n", res);
            return NULL;
        }
        switch (signal) {
            case SIGINT:{
                appendOnLog(ServerLog, "[SignalHandler]: %d signal received, quitting\n", SIGINT);
                if(writen(pipeFd, &signal, sizeof(int)) == -1){
                    fprintf(stderr, "ERROR - SigHandler Pipe writing failure\n");
                }
                ServerStorage->status = S;
                close(pipeFd);
                return NULL;
            }
            case SIGQUIT:{
                appendOnLog(ServerLog, "[SignalHandler]: %d signal received, "
                                       "quitting after serving last clients\n", SIGQUIT);
                if(writen(pipeFd, &signal, sizeof(int)) == -1){
                    fprintf(stderr, "ERROR - SigHandler Pipe writing failure\n");
                }
                ServerStorage->status = Q;
                close(pipeFd);
                return NULL;
            }
            case SIGHUP:{
                appendOnLog(ServerLog, "[SignalHandler]: %d signal received, "
                                       "quitting after serving last clients\n", SIGHUP);
                if(writen(pipeFd, &signal, sizeof(int)) == -1){
                    fprintf(stderr, "ERROR - SigHandler Pipe writing failure\n");
                }
                ServerStorage->status = Q;
                close(pipeFd);
                return NULL;
            }
        }
    }
}
int signalHandlerDestroy(){
    appendOnLog(ServerLog, "[SignalHandler]: Stopped\n");
    return pthread_join(sigHandler_t, NULL);
}

static int serverConfigParser(char* path){


    /* Define variables */
    int c = 0, j = 0;
    FILE *configFile;
    char* token, *rest;
    char fileBuffer[256];
    int socketSet = 0, policySet = 0;
    /* Config variables */
    long threads = 0, max_file = 0, debug = -1;
    long max_bytes = 0;
    /* Try to open file */
    if(path == NULL) {
        defaultConfig(&ServerConfig);
        return 0;
    }
    if((configFile = fopen(path, "r") )== NULL){
        free(path);
        return -1;
    }

    /* If file opened, try to parse it */
    while (fgets(fileBuffer, 256, configFile) != NULL){
        token = strtok_r(fileBuffer, ":", &rest);
        if(token == NULL) {
            SYSCALL_RETURN(config_fclose, fclose(configFile),
                           "ERROR - Closing file, errno = %d", errno)
            return -1;
        }

        if(strncmp(token, "Files", 5) == 0){
            if(max_file == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                SYSCALL_RETURN(Serverconfig_stringToLong_1, (max_file = StringToLong(token)),
                               "ERROR - String to long conversion, errno = %d", errno)
                ServerConfig.maxFile = max_file;
            } else{
                SYSCALL_RETURN(config_fclose, fclose(configFile),
                               "ERROR - Closing file, errno = %d", errno)
                fprintf(stderr, "ERROR - Files must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strncmp(token, "Bytes", 5) == 0){
            if(max_bytes == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                SYSCALL_RETURN(ServerConfig_stringToLong_2, (max_bytes = StringToLong(token)),
                               "ERROR - String to long conversion, errno = %d", errno)
                ServerConfig.maxByte = max_bytes;
            } else{
                SYSCALL_RETURN(config_fclose, fclose(configFile),
                               "ERROR - Closing file, errno = %d", errno)
                fprintf(stderr, "ERROR - Bytes must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strncmp(token, "Threads", 7) == 0){
            if(threads == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                SYSCALL_RETURN(ServerConfig_stringToLong_2, (threads = StringToLong(token)),
                               "ERROR - String to long conversion, errno = %d", errno)
                ServerConfig.threadNumber = threads;
            } else{
                SYSCALL_RETURN(config_fclose, fclose(configFile),
                               "ERROR - Closing file, errno = %d", errno)
                fprintf(stderr, "ERROR - Threads must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strncmp(token, "Debug", 5) == 0){
            if(debug == -1){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                /* De blank string */
                c = 0, j = 0;
                while(token[c]!='\0'){
                    if(token[c]!=' '){
                        token[j++]=token[c];
                    }
                    c++;
                }
                token[j]='\0';
                if(strcmp(token, "True")==0 || strcmp(token, "true")==0) debug = 1;
                else debug = 0;
            } else{
                SYSCALL_RETURN(config_fclose, fclose(configFile),
                               "ERROR - Closing file, errno = %d", errno)
                fprintf(stderr, "ERROR - Debug must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strncmp(token, "Policy", 6) == 0){
            if( policySet == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                /* De blank string */
                c = 0, j = 0;
                while(token[c]!='\0'){
                    if(token[c]!=' '){
                        token[j++]=token[c];
                    }
                    c++;
                }
                token[j]='\0';
                policySet = 1;
                ServerConfig.policy = fromStringToEnumCachePolicy(token);
            } else{
                SYSCALL_RETURN(config_fclose, fclose(configFile),
                               "ERROR - Closing file, errno = %d", errno)
                fprintf(stderr, "ERROR - Policy must be set once, multiple assignments detected\n");
                return -1;
            }
        }
        else if(strncmp(token, "Socket", 6) == 0){
            if(socketSet == 0) {
                token = strtok_r(NULL, "\n\0", &rest);
                if (token == NULL) break;

                /* De blank string */
                c = 0, j = 0;
                while(token[c]!='\0'){
                    if(token[c]!=' '){
                        token[j++]=token[c];
                    }
                    c++;
                }
                token[j]='\0';

                strncpy(ServerConfig.serverSocketName, token , UNIX_PATH_MAX - 1);
                socketSet = 1;
            }
        }
    }

    if(socketSet == 0){
        strncpy(ServerConfig.serverSocketName, "./tmp/cs_socket", UNIX_PATH_MAX);
    }
    if(threads == 0 || max_file == 0 || max_bytes == 0){
        fprintf(stderr, "ERROR - Config file wrong parsing");
        return -1;
    }
    if(debug == -1) debug = 0;
    SYSCALL_RETURN(config_fclose, fclose(configFile),
                   "ERROR - Closing file, errno = %d", errno)
    return debug;
}
int serverInit(char* configPath, char* logPath){
    int debug;
    if ((debug = serverConfigParser(configPath)) == -1) {
        fprintf(stderr, "ERROR - Parsing config, err = %d", errno);
    }
    createLog(logPath, &ServerLog);

    ServerStorage = malloc(sizeof (fileServer));
    ServerStorage->stdOutput = debug;
    ServerStorage->status = E;

    ServerStorage->connected = 0;               /* Int */
    ServerStorage->deletedBytes = 0;            /* size_t */
    ServerStorage->deletedFiles = 0;            /* int */
    ServerStorage->expelledFiles = 0;           /* size_t */
    ServerStorage->expelledBytes = 0;           /* Int */
    ServerStorage->maxConnections = 0;          /* Int */
    ServerStorage->sessionMaxBytes = 0;         /* Size_t */
    ServerStorage->actualFilesBytes = 0;        /* Size_t */
    ServerStorage->actualFilesNumber = 0;       /* Int */
    ServerStorage->sessionMaxFilesNumber = 0;   /* Int */

    ServerStorage->filesTable = icl_hash_create((2*ServerConfig.maxFile), NULL,NULL);
    if(!ServerStorage->filesTable) {
        free(ServerStorage);
        errno = ENOMEM;
        return -1;
    }

    if(pthread_mutex_init(&(ServerStorage->lock), NULL) != 0){
        SYSCALL_RETURN(ServerInit_hashDestroy, icl_hash_destroy(ServerStorage->filesTable, free, free),
                       "ERROR - Icl_Hash destroy fault, errno = %d", errno);
        free(ServerStorage);
        return -1;
    }

    appendOnLog(ServerLog, "MAIN: Server config initialized\n");
    appendOnLog(ServerLog, "Max_Files: %d\n", ServerConfig.maxFile);
    appendOnLog(ServerLog, "Max_Bytes: %d\n", ServerConfig.maxByte);
    appendOnLog(ServerLog, "Thread: %d\n", ServerConfig.threadNumber);
    appendOnLog(ServerLog, "Replacement Policy: %s\n", fromEnumCachePolicyToString(ServerConfig.policy));
    appendOnLog(ServerLog, "Socket: %s\n", ServerConfig.serverSocketName);
    logSeparator(ServerLog);
    return 0;
}
void serverDestroy(){
    int res;
    if(stopThreadPool(threadPool1, (ServerStorage->status==S)? 1:0) != 0){
        printf("Error - StopThreadPool, %d", errno);
    }
    signalHandlerDestroy();
    free(sigHandler_Pipe);
    printServerStatus();
    closeLogStr(ServerLog);
    if(pthread_mutex_destroy(&(ServerStorage->lock)) != 0){
        res =icl_hash_destroy(ServerStorage->filesTable, free, freeFile);
        if(res == -1 && ServerStorage->stdOutput) printf("ERROR - Icl_Hash destroy fault, errno = %d", errno);
        free(ServerStorage);
        exit(0);
    }
    res = icl_hash_destroy(ServerStorage->filesTable, free, freeFile);
    if(res == -1 && ServerStorage->stdOutput) printf("ERROR - Icl_Hash destroy fault, errno = %d", errno);
    free(ServerStorage);
}
void printServerStatus(){
    int i, j;
    struct tm cTime, lTime;
    serverFile* file;
    icl_entry_t *bucket, *curr, *tmp;
    char* max,* actual,* expelled,* deletedBytes,* serverMaxBytes;
    for (i = 0; i<ServerStorage->filesTable->nbuckets; i++) {
        bucket = ServerStorage->filesTable->buckets[i];
        for(curr = bucket, tmp=bucket; tmp!=NULL;){
            tmp=curr->next;
            file = (serverFile*)curr->data;
            if(file->toDelete!=0) {
                curr=curr->next;
                icl_hash_delete(ServerStorage->filesTable, file->path, free, freeFile);
                continue;
            }else if(file->latsOp==O_CREAT_LOCK || file->latsOp==O_CREAT) {
                fileNumDecrement();
                ServerStorage->sessionMaxFilesNumber--;
                curr=curr->next;
                icl_hash_delete(ServerStorage->filesTable, file->path, free, freeFile);
                continue;
            } else if(file->latsOp==O_WRITETOCLOSE) file->latsOp=O_WRITE;
            curr=curr->next;
        }
    }

    logSeparator(ServerLog);
    max = calculateSize(ServerStorage->sessionMaxBytes);
    expelled = calculateSize(ServerStorage->expelledBytes);
    actual = calculateSize(ServerStorage->actualFilesBytes);
    serverMaxBytes = calculateSize(ServerConfig.maxByte);
    deletedBytes = calculateSize(ServerStorage->deletedBytes);
    appendOnLog(ServerLog,"        Max client: %d\n", ServerStorage->maxConnections);
    appendOnLog(ServerLog,"        Max number of files saved: %d\n", ServerStorage->sessionMaxFilesNumber);
    appendOnLog(ServerLog,"        Max bytes of files saved: %s %d B\n", max, (int)ServerStorage->sessionMaxBytes);
    appendOnLog(ServerLog,"        Number of files expelled: %d\n", ServerStorage->expelledFiles);
    appendOnLog(ServerLog,"        Number of bytes expelled: %s %d B\n", expelled, (int)ServerStorage->expelledBytes);
    appendOnLog(ServerLog,"        Number of files deleted: %d\n", ServerStorage->deletedFiles);
    appendOnLog(ServerLog,"        Number of bytes deleted: %s %d B\n", deletedBytes, (int)ServerStorage->deletedBytes);
    appendOnLog(ServerLog,"        Files on the server at shutdown %d of %d\n", ServerStorage->actualFilesNumber, ServerConfig.maxFile);
    appendOnLog(ServerLog,"        Bytes on the server at shutdown %s of %s (%d of %d):\n\n", actual, serverMaxBytes, (int)ServerStorage->actualFilesBytes, (int)ServerConfig.maxByte);
    fprintf(stdout, "\n--------------------------------------------------------------------------------------------------\n");
    fprintf(stdout, "        Max client: %d\n", ServerStorage->maxConnections);
    fprintf(stdout, "        Max number of files saved: %d\n", ServerStorage->sessionMaxFilesNumber);
    fprintf(stdout, "        Max bytes of files saved: %s %d\n", max, (int)ServerStorage->sessionMaxBytes);
    fprintf(stdout, "        Number of files expelled: %d\n", ServerStorage->expelledFiles);
    fprintf(stdout, "        Number of bytes expelled: %s %d B\n", expelled, (int)ServerStorage->expelledBytes);
    fprintf(stdout, "        Number of files deleted: %d\n", ServerStorage->deletedFiles);
    fprintf(stdout, "        Number of bytes deleted: %s %d B\n", deletedBytes, (int)ServerStorage->deletedBytes);
    fprintf(stdout, "        Files on the server at shutdown %d of %d\n", ServerStorage->actualFilesNumber, ServerConfig.maxFile);
    fprintf(stdout, "        Bytes on the server at shutdown %s of %s (%d of %d):\n\n", actual, serverMaxBytes, (int)ServerStorage->actualFilesBytes,(int) ServerConfig.maxByte);
    free(max);
    free(expelled);
    free(actual);
    free(serverMaxBytes);
    free(deletedBytes);
    if(ServerStorage->actualFilesNumber>0){
        appendOnLog(ServerLog,"        Files :\n");
        appendOnLog(ServerLog,"        Size --- Last Operation & Time --- Create Time --- Path\n");
        fprintf(stdout, "        Files :\n");
        fprintf(stdout, "        Size --- Last Operation & Time --- Create Time --- Path\n");
        j = 0;
        for (i = 0; i<ServerStorage->filesTable->nbuckets; i++) {
            bucket = ServerStorage->filesTable->buckets[i];
            for(curr = bucket; curr!=NULL; curr=curr->next){
                j++;
                file = (serverFile*)curr->data;
                actual = calculateSize((int)file->size);
                localtime_r(&(file->creationTime.tv_sec), &cTime);
                localtime_r(&(file->lastOpTime.tv_sec), &lTime);
                /* if(file->latsOp==O_CREAT_LOCK||file->latsOp==O_CREAT ) continue; */
                appendOnLog(ServerLog,"        %d %s --- %s %d/%d/%d %d:%d:%d --- %d/%d %d:%d:%d --- %s\n", j, actual, requestToString(file->latsOp), lTime.tm_mday, lTime.tm_mon+1, lTime.tm_year+1900, lTime.tm_hour, lTime.tm_min, lTime.tm_sec, cTime.tm_mday, cTime.tm_mon+1, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, file->path);
                fprintf(stdout, "        %d %s --- %s %d/%d/%d %d:%d:%d --- %d/%d %d:%d:%d --- %s\n", j, actual, requestToString(file->latsOp), lTime.tm_mday, lTime.tm_mon+1, lTime.tm_year+1900, lTime.tm_hour, lTime.tm_min, lTime.tm_sec, cTime.tm_mday, cTime.tm_mon+1, cTime.tm_hour, cTime.tm_min, cTime.tm_sec, file->path);
                free(actual);
            }
        }
    }
    fprintf(stdout, "--------------------------------------------------------------------------------------------------\n");
}

int tSpecCmp(struct timespec time1, struct timespec time0) {
    return (time1.tv_sec == time0.tv_sec)? time1.tv_nsec > time0.tv_nsec : time1.tv_sec > time0.tv_sec;
}

serverFile* replaceFile(serverFile* file1, serverFile* file2){
    if(file1 == NULL && file2 == NULL) return NULL;
    if(file1 == NULL) return file2;
    if(file2 == NULL) return file1;
    /*
     * struct tm* cTime1 = gmtime(&(file1->creationTime.tv_sec));
     * struct tm* cTime2 = gmtime(&(file2->creationTime.tv_sec));
     * printf("file1: %s %d:%d:%d,file2: %s %d:%d:%d, %ld\n", file1->path, cTime1->tm_hour, cTime1->tm_min, cTime1->tm_sec, file2->path, cTime2->tm_hour, cTime2->tm_min, cTime2->tm_sec, file1->creationTime.tv_sec-file2->creationTime.tv_sec);
    */
    switch (ServerConfig.policy) {
        case FIFO:
            if (tSpecCmp(file1->creationTime, file2->creationTime)) {
                return file1;
            } else {
                return file2;
            }
        case LIFO:
            if (tSpecCmp(file1->creationTime, file2->creationTime)) {
                return file2;
            } else {
                return file1;
            }
        case LRU:
            if (tSpecCmp(file1->lastOpTime, file2->lastOpTime)) {
                return file1;
            } else {
                return file2;
            }
        case MRU:
            if (tSpecCmp(file1->lastOpTime, file2->lastOpTime)) {
                return file2;
            } else {
                return file1;
            }
        default: return NULL;
    }
}

serverFile * icl_hash_toReplace(icl_hash_t *ht, int workerId){
    int i;
    icl_entry_t *bucket, *curr;
    serverFile *file1, *file2 = NULL, *exFile2 = NULL;
    for (i = 0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr = bucket; curr!=NULL; curr=curr->next){
            file1= (serverFile*) curr->data;
            if(!file1 || file1->toDelete != 0 || file1->lockFd != -1 || file1->latsOp==O_CREAT_LOCK || file1->latsOp==O_CREAT || file1->latsOp==O_WRITETOCLOSE) continue;
            fileReadersIncrement(file1, workerId);
            if(file2 && file1!=file2) fileReadersIncrement(file2, workerId);
            exFile2 = file2;
            file2 = replaceFile(file1, file2);
            fileReadersDecrement(file1, workerId);
            if(file2 && exFile2 && (file2 != exFile2 || file1!=file2)) {
                fileReadersDecrement(exFile2, workerId);
            }
        }
    }
    return file2;
}

char* fromEnumCachePolicyToString(cachePolicy policy){
    switch (policy) {
        case FIFO: return "FIFO";
        case LIFO: return "LIFO";
        case LRU: return "LRU";
        case MRU: return "MRU";
        default: return NULL;
    }
}

cachePolicy fromStringToEnumCachePolicy(char* str){
    if(strncmp(str, "FIFO", 4) == 0) return FIFO;
    if(strncmp(str, "LIFO", 4) == 0) return LIFO;
    if(strncmp(str, "LRU", 3) == 0) return LRU;
    if(strncmp(str, "MRU", 3) == 0) return MRU;
    return FIFO;
}

void freeFile(void* file) {
    serverFile *file1 = (serverFile *) file;
    if(file1->size>0){
        if (file1->content != NULL) free(file1->content);
    }
    if (file1->clients_fd != NULL) deleteList(&(file1->clients_fd));
    pthread_mutex_destroy(&(file1->lock));
    pthread_mutex_destroy(&(file1->order));
    pthread_cond_destroy(&(file1->condition));
    free(file);
}

void fileNumIncrement(){
    ServerStorage->actualFilesNumber++;
    ServerStorage->sessionMaxFilesNumber++;
}

void fileNumDecrement(){
    if(ServerStorage->actualFilesNumber>0) ServerStorage->actualFilesNumber--;
}

void fileBytesIncrement(size_t size){
    ServerStorage->actualFilesBytes += size;
    ServerStorage->sessionMaxBytes += size;
}

void fileBytesDecrement(size_t size){
    if(ServerStorage->actualFilesBytes>0) ServerStorage->actualFilesBytes -= size;
}

void defaultConfig(serverConfig* config){
    config->maxFile = 100;
    config->maxByte = 15728640;
    config->threadNumber = 15;
    config->policy = FIFO;
    strncpy(config->serverSocketName, "./tmp/cs_socket", UNIX_PATH_MAX);
}

void newClient(){
    ServerStorage->connected++;
    if(ServerStorage->connected>ServerStorage->maxConnections){
        ServerStorage->maxConnections = ServerStorage->connected;
    }
}

void removeClient(){
    if(ServerStorage->connected>0) ServerStorage->connected--;
}

/**
 * Search for an entry in a hash table.
 *
 * @param ht -- the hash table to be searched
 * @param key -- the key of the item to search for
 *
 * @returns pointer to the data corresponding to the key.
 *   If the key was not found, returns NULL.
 */
void* icl_hash_find(icl_hash_t *ht, void* key)
{
    icl_entry_t* curr;
    unsigned int hash_val;

    if(ServerStorage->actualFilesNumber==0) return NULL;
    if(!ht || !key) return NULL;
    hash_val = (* ht->hash_function)(key) % ht->nbuckets;

    for (curr=ht->buckets[hash_val]; curr != NULL; curr=curr->next)
        if ( ht->hash_key_compare(curr->key, key)){
            return(curr->data);
        }
    return NULL;
}


int icl_hash_toDelete(icl_hash_t * ht, List expelled, int fd, int workerId){
    icl_entry_t *bucket, *curr;
    serverFile* file;
    int i;

    if(!ht) return 0;
    for(i=0; i<ht->nbuckets; i++) {
        bucket = ht->buckets[i];
        for(curr=bucket; curr!=NULL; curr = curr->next) {
            file = ((serverFile*)curr->data);
            if(curr->key){
                if(file->writers>0 || file->latsOp==O_CREAT_LOCK || file->latsOp==O_CREAT) {
                    continue;
                }
                if(file->toDelete==fd){
                    fileWritersIncrement(file, workerId);
                    pushTop(&expelled, file->path, NULL);
                    file->toDelete = 1;
                    fileWritersDecrement(file, workerId);
                }
            }
        }
    }

    return expelled->len;
}
