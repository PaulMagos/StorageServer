//
// Created by paul magos on 11/01/22.


#include "../../headers/server.h"

// -------------------------------- GLOBAR VARIABLES --------------------------------
serverConfig ServerConfig;
fileServer* ServerStorage;
logFile ServerLog;
clientList* fd_clients;

int clientInit();
int clientDestroy();
int clientGetCount();
int clientAdd(int fd);
void* signalHandler(void* arguments);
int serverInit(char* configPath, char* logPath);

int main(int argc, char* argv[]){
    if(argc == 0){
        serverInit(NULL, NULL);
        fprintf(stderr, "No arguments, Server will start with default parameters\n");
    }else if(argc == 1){
        serverInit(argv[0], NULL);
    }else if(argc == 2){
        serverInit(argv[0], argv[1]);
    }else if(argc>2){
        fprintf(stderr, "ERROR - Too many argument, max 2 arguments: \n");
        printf("Example: ./server config_path log_directory\n");
    }

    // Signal Set creation for the sigHandler thread
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    int res = 0;
    SYSCALL_EXIT(pthread_sigmask, res, pthread_sigmask(SIG_BLOCK, &mask, NULL),
                 "ERROR - pthread_sigmask fault, errno = %d\n", errno)

    // SigPipe Ignore
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    SYSCALL_EXIT(sigaction, res, sigaction(SIGPIPE, &act, NULL),
                 "ERROR - sigaction failure to ignore sigpipe, errno = %d\n", errno)
    // Pipe init for threads communications
    int pipeFd[2];
    SYSCALL_EXIT(pipe, res, pipe(pipeFd),
                 "ERROR - pipe failure, errno = %d", errno)

    // Sig Handler Thread Creation
    pthread_t sigHandler_t;
    sigHandlerArgs sigArgs = {pipeFd[1], &mask};
    SYSCALL_EXIT(sigHandler_pthread_creation, res, pthread_create(&sigHandler_t, NULL, signalHandler, &sigArgs),
                 "ERROR - Signal Handler Thread Creation Failure, errno = %d", errno)

    // ThreadPool Pipe
    int threadsPipe[2];
    SYSCALL_EXIT(thradspipe, res, pipe(threadsPipe),
                 "ERROR - Pipe init failure")

    // ThreadPool Initialize
    threadPool* threadPool1 = initThreadPool(ServerConfig.threadNumber);
    if(!threadPool1){
        fprintf(stderr, "ERROR - threadPool init failure");
        exit(EXIT_FAILURE);
    }


    // Socket creation
    int max_fd;
    int fd_server_socket, fd_client_socket;
    struct sockaddr_un SocketAddress;
    (void) unlink(ServerConfig.serverSocketName);
    memset(&SocketAddress, '0', sizeof(SocketAddress));
    strncpy(SocketAddress.sun_path, ServerConfig.serverSocketName, UNIX_PATH_MAX);
    SocketAddress.sun_family = AF_UNIX;

    // Socket binding and ready to listen
    SYSCALL_EXIT(socket, fd_server_socket, socket(AF_UNIX, SOCK_STREAM, 0),
            "ERROR - Socket set failure")
    SYSCALL_EXIT(bind, res, bind(fd_server_socket, (struct sockaddr*) &SocketAddress, sizeof(SocketAddress)),
            "ERROR - Socket bind failure")
    SYSCALL_EXIT(listen, res, listen(fd_server_socket, SOMAXCONN),
            "ERROR - Socket listen failure")


    fd_set readySet, allSet;
    FD_ZERO(&allSet);
    FD_SET(fd_server_socket, &allSet);
    FD_SET(pipeFd[0], &allSet);
    FD_SET(threadsPipe[0], &allSet);

    max_fd = max(fd_server_socket, pipeFd[0]);
    max_fd = max(max_fd, threadsPipe[0]);


    struct timeval temp;
    while (ServerStorage->status == E){
        readySet = allSet;
        temp = {0, 200000}; // Time 2 seconds
        if((res = select(max_fd+1, &readySet, NULL, NULL, &temp) ) < 0 ){
            fprintf(stderr, "ERROR - ")
        }
    }


}


static int serverConfigParser(char* path){
    // Test arguments
    if(path == NULL) {
        errno = EINVAL;
        return -1;
    }

    // Define variables
    FILE *configFile;
    char* token, *rest;
    char fileBuffer[256];
    int socketSet = 0;
    // Config variables
    long threads = 0, max_file = 0, max_bytes = 0;

    // Try to open file
    if((configFile = fopen(path, "r") )== NULL){
        return -1;
    }

    // If file opened, try to parse it
    while (fgets(fileBuffer, 256, configFile) != NULL){
        token = strtok_r(fileBuffer, ":", &rest);
        if(token == NULL) return -1;

        if(strcmp(token, "Files", 5) == 0){
            if(max_file == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                SYSCALL_RETURN(Serverconfig_stringToLong_1, (max_file = StringToLong(token)),
                             "ERROR - String to long conversion, errno = %d", errno)
                ServerConfig.maxFile = max_file;
            } else{
                fprintf(stderr, "ERROR - Files must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strcmp(token, "Bytes", 5) == 0){
            if(max_bytes == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                SYSCALL_RETURN(ServerConfig_stringToLong_2, (max_bytes = StringToLong(token)),
                               "ERROR - String to long conversion, errno = %d", errno)
                ServerConfig.maxFile = max_file;
            } else{
                fprintf(stderr, "ERROR - Bytes must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strcmp(token, "Threads", 7) == 0){
            if(threads == 0){
                token = strtok_r(NULL, "\n\0", &rest);
                if(token == NULL) return -1;
                SYSCALL_RETURN(ServerConfig_stringToLong_2, (max_bytes = StringToLong(token)),
                               "ERROR - String to long conversion, errno = %d", errno)
                ServerConfig.threadNumber = threads;
            } else{
                fprintf(stderr, "ERROR - Threads must be set once, multiple assignments detected\n");
                return -1;
            }
        } else if(strcmp(token, "Socket", 6) == 0){
            if(socketSet == 0) {
                token = strtok_r(NULL, "\n\0", &rest);
                if (token == NULL) return -1;
                int i = 0;
                while (token[i] == ' ') i++;
                if (token[i] == '\0') {
                    // In case it is empty I'll choose the default socket
                    continue;
                }
                strncpy(ServerConfig.serverSocketName, token + i, UNIX_PATH_MAX - 1);
                socketSet = 1;
            }
        }
    }

    if(threads == 0 || max_file == 0 || max_bytes == 0 || socketSet == 0){
        fprintf(stderr, "ERROR - Config file wrong parsing");
        return -1;
    }
    SYSCALL_RETURN(config_fclose, fclose(configFile),
                   "ERROR - Closing file, errno = %d", errno)
    return 0;
}
int clientInit(){
    fd_clients = malloc(sizeof(clientList));
    if(fd_clients == NULL) return -1;
    SYSCALL_RETURN(listCreation, createList(&(fd_clients->ClientsFds)),
                   "ERROR - Client fd list creation, errno = %d\n", errno)
    SYSCALL_RETURN(pthread_mutex_init, pthread_mutex_init(fd_clients->lock),
                   "ERROR - Mutex init Failure clients_fd structure\n")
    return 0;
}
int clientAdd(int fd){
    SYSCALL_RETURN(pthread_mutex_lock, pthread_mutex_lock(&(fd_clients->lock)),
                   "ERROR - fd_clients lock failure, errno = %d\n", errno)

    if(compareDataAsInt(fd_clients->ClientsFds->head, fd) == 1){
        errno = EISCONN;
        SYSCALL_RETURN(pthread_mutex_unlock, pthread_mutex_unlock(&(fd_clients->lock)),
                       "ERROR - fd_clients unlock failure, errno = %d\n", errno)
        return -1;
    }

    int* newFd = malloc(sizeof(int));
    if(newFd == NULL){
        SYSCALL_RETURN(pthread_mutex_unlock, pthread_mutex_unlock(&(fd_clients->lock)),
                       "ERROR - fd_clients unlock failure, errno = %d\n", errno)
        return -1;
    }

    if(pushBottom(&(fd_clients->ClientsFds), NULL, newFd) != 0){
        SYSCALL_RETURN(pthread_mutex_unlock, pthread_mutex_unlock(&(fd_clients->lock)),
                       "ERROR - fd_clients unlock failure, errno = %d\n", errno)
        return -1;
    }
    SYSCALL_RETURN(pthread_mutex_unlock, pthread_mutex_unlock(&(fd_clients->lock)),
                   "ERROR - fd_clients unlock failure, errno = %d\n", errno)
    return 0;
}
int clientGetCount(){
    SYSCALL_RETURN(pthread_mutex_lock, pthread_mutex_lock(&(fd_clients->lock)),
                   "ERROR - fd_clients lock failure, errno = %d\n", errno)
    int tmp = fd_clients->ClientsFds->len;
    SYSCALL_RETURN(pthread_mutex_unlock, pthread_mutex_unlock(&(fd_clients->lock)),
                   "ERROR - fd_clients unlock failure, errno = %d\n", errno)
    return tmp;
}
int clientDestroy(){
    void * fdToClose;
    while (pullTop(&(fd_clients->ClientsFds), NULL, &fdToClose) == 0){
        SYSCALL_RETURN(fdClose, close(*(int*)(fdToClose)),
                       "ERROR - Closing fd %d", *(int*)(fdToClose));
    }
    SYSCALL_RETURN(deleteList, deleteList(&(fd_clients->ClientsFds)),
                   "ERROR - Deleting client list, errno = %d", errno);
    pthread_mutex_destroy(&(fd_clients->lock));
    free(fd_clients);
    return 0;
}
int serverInit(char* configPath, char* logPath){
    if(configPath != NULL){
        serverConfigParser(configPath);
    } else ServerConfig = DEFAULT_CONFIG;

    if(logPath == NULL){
        createLog(NULL, &ServerLog);
    } else{
        createLog(logPath, &ServerLog);
    }

    if(clientInit() != 0){
        free(fd_clients);
        return -1;
    }


    ServerStorage->stdOutput = 0;
    ServerStorage->status = E;

    ServerStorage->expelledFiles = 0;           // Int
    ServerStorage->maxConnections = 0;          // Int
    ServerStorage->sessionMaxBytes = 0;         // Size_t
    ServerStorage->actualFilesBytes = 0;        // Size_t
    ServerStorage->actualFilesNumber = 0;       // Int
    ServerStorage->sessionMaxFilesNumber = 0;   // Int

    ServerStorage->filesTable = icl_hash_create((2*ServerConfig.maxFile), NULL,NULL);
    if(!ServerStorage->filesTable) {
        free(ServerStorage);
        errno = ENOMEM;
        return -1;
    }

    if(pthread_mutex_init(&(ServerStorage->lock), NULL) != 0){
        free(ServerStorage);
        SYSCALL_RETURN(ServerInit_hashDestroy, icl_hash_destroy(ServerStorage->filesTable, free, free),
                       "ERROR - Icl_Hash destroy fault, errno = %d", errno);
        return -1;
    }

    return 0;
}
void* signalHandler(void *arguments){
    sigset_t *workingSet = ((sigHandlerArgs*)arguments)->sigSet;
    int pipeFd = ((sigHandlerArgs*)arguments)->pipe;
    int signal;
    int res;
    while(true){
        SYSCALL_ASSIGN(sigwait, res, sigwait(workingSet, &signal)),
                       "ERROR - sigwait failure, errno = %d", res);
        switch (signal) {
            case SIGINT:
            case SIGQUIT:{
                appendOnLog(ServerLog, "Signal Handler: %s signal received, quitting\n",
                            (signal==SIGQUIT)? SIGQUIT:SIGINT);
                if(writen(pipeFd, &signal, sizeof(int)) == -1){
                    fprintf(stderr, "ERROR - SigHandler Pipe writing failure\n");
                    return;
                }
                close(pipeFd);
                return ;
            }
            case SIGHUP:{
                appendOnLog(ServerLog, "Signal Handler: %s signal received, quitting after serving last clients\n",
                            (signal==SIGQUIT)? SIGQUIT:SIGINT);
                if(writen(pipeFd, &signal, sizeof(int)) == -1){
                    fprintf(stderr, "ERROR - SigHandler Pipe writing failure\n");
                    return;
                }
                close(pipeFd);
                return ;
            }
            default:{
                ;
            }
        }
    }
    return;
}