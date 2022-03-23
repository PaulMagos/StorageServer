/*
 * Created by paul magos on 16/01/22.
 */

#include "../../headers/api.h"

/* Socket name */
const char* sName;
/* Socket fd */
int fd_socket = -1;


int lengthCheck(const char* str){
    if(strlen(str) + 1 > UNIX_PATH_MAX) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int diffTimespec(struct timespec time1, struct timespec time0) {
    return (time1.tv_sec == time0.tv_sec)? time1.tv_nsec > time0.tv_nsec : time1.tv_sec > time0.tv_sec;
}

struct timespec diff_timespec(const struct timespec time1,
                              const struct timespec time0) {
    struct timespec diff = {time1.tv_sec - time0.tv_sec,time1.tv_nsec - time0.tv_nsec};
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1000000000;
        diff.tv_sec--;
    }
    return diff;
}

int saveIntoDir(const char* dir, int numOfFiles);
int storeFile(char* pathname, const char* saveDir, size_t size, void** buf);
int mkpath(char* file_path, mode_t mode);

int openConnection(const char* sockname, int msec, const struct timespec abstime){


    /* SYS CALL RESPONSE ( "Re-Used" for other functions when needed ) */
    int scRes = -1;
    int connRes;
    /* Socket address */
    struct sockaddr_un socketAdd;
    /* Time variables */
    struct timespec currenTime, iniTime;

    if(!sockname || msec<0){
        /* INVALID ARGUMENTS */
        errno = EINVAL;
        return -1;
    }
    memset(&socketAdd, '0', sizeof(socketAdd));

    SYSCALL_EXIT(lengthCheck, scRes, lengthCheck(sockname), "ERROR - socket name too long, errno = %d\n", errno);
    strncpy(socketAdd.sun_path, sockname, UNIX_PATH_MAX);
    socketAdd.sun_family = AF_UNIX;

    SYSCALL_ASSIGN(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &iniTime),
                 "ERROR - initializing time, errno = %d\n", errno);
    if(scRes==-1){
        return -1;
    }
    SYSCALL_ASSIGN(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &currenTime),
                 "ERROR - getting current time, errno = %d\n", errno);
    if(scRes==-1){
        return -1;
    }
    SYSCALL_ASSIGN(socket, fd_socket, socket(AF_UNIX, SOCK_STREAM, 0),
                 "ERROR - socket connection failed, errno = %d\n", errno);
    if(fd_socket==-1){
        return -1;
    }
    while ((diff_timespec( abstime, diff_timespec(currenTime, iniTime))).tv_sec
        && (connRes = connect(fd_socket, (struct sockaddr*)&socketAdd, sizeof(socketAdd))) == -1){
        msleep(msec);
        SYSCALL_ASSIGN(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &currenTime),
                     "ERROR - getting current time, errno = %d\n", errno);
        if(scRes==-1){
            return -1;
        }
        /* fprintf(stdout, "Trying connection... %d\n", (int)(currenTime.tv_sec-iniTime.tv_sec+1)); */
    }

    if(connRes==-1){
        fd_socket = -1;
        errno = ETIMEDOUT;
        return -1;
    }
    sName = sockname;
    return 0;
}

int closeConnection(const char* sockname){
    int scRes;
    if(!sockname){
        errno = EINVAL;
        return -1;
    }
    /* if it's not the socket we are connected to, gives an error */
    if(strcmp(sName, sockname)!=0){
        errno = ENOTCONN;
        return -1;
    }
    sName = NULL;
    SYSCALL_EXIT(close, scRes, close(fd_socket), "ERROR - closing connection to %s, errno = %d\n", sockname, errno);
    fd_socket = -1;
    return 0;
}

int openFile(const char* pathname, int flags){
    int scRes = 0;
    message msg;

    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = (flags==O_LOCK)? O_CREAT_LOCK:flags;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes == -1){
        freeMessageContent(&msg);
        return -1;
    }

    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes == -1){
        return -1;
    }
    if(msg.feedback!=SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    /* File aperto correttamente */
    freeMessageContent(&msg);
    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){
    int scRes;
    message msg;

    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }

    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_READ;
    msg.feedback = 0;
    msg.additional = 0;

    /* Send request for reading file */
    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ReadFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ReadFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        return -1;
    }
    /* If any Error return */
    if(msg.feedback!=SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);

    /* Receive File */
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        return -1;
    }
    if(msg.feedback!=SUCCESS && msg.request!=O_DATA){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    *buf = malloc(msg.size);
    *size = msg.size;
    memcpy( *buf, msg.content, msg.size);
    freeMessageContent(&msg);
    return 0;
}

int readNFiles(int N, const char* dirname){
    int scRes;
    message msg;
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }

    memset(&msg, 0, sizeof(message));
    msg.size = 0;
    msg.content = NULL;
    msg.request = O_READN;
    msg.feedback = 0;
    msg.additional = N;


    /* Send request for reading file */
    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        return -1;
    }
    if(msg.feedback!=SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    /* Salvo i file se devono essere salvati, li leggo altrimenti */
    if(saveIntoDir(dirname, msg.additional) == -1) {
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    return 0;
}

int writeFile(const char* pathname, const char* dirname){
    struct stat fileDet;
    size_t fileSize;
    void* fileBuf;
    FILE* fil3;
    int scRes = 0;
    message msg;
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }

    /*
     * LINUX/Posix stat library for retrieving a path info,
     * I could've used standard library by seeking to the end of the file and back,
     * but I've preferred this method
     * https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
     */
    if(stat(pathname, &fileDet) == -1){
        errno = EINVAL;
        return -1;
    }
    fileSize = fileDet.st_size;

    /* Alloco un buffer della dimensione del file e verifico che sia andata a buon fine l'allocazione */
    fileBuf = malloc(fileSize);
    if(fileBuf == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Tento l'apertura del file in lettura binaria e ne verifico l'esito */
    if((fil3 = fopen(pathname, "rb")) == NULL){
        return -1;
    }
    /* Leggo il file nel buffer e verifico la dimensione dell'entità letta dalla "fread" */
    if(fread(fileBuf, 1, fileSize, fil3) < fileSize){
        free(fileBuf);
        errno = EIO;
        fclose(fil3);
        return -1;
    }

    if(fclose(fil3) != 0 ) return -1;


    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_WRITE;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        free(fileBuf);
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        free(fileBuf);
        return -1;
    }
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        free(fileBuf);
        freeMessageContent(&msg);
        return -1;
    }

    freeMessageContent(&msg);

    msg.size = fileSize;
    msg.content = malloc(fileSize);
    memcpy(msg.content, fileBuf, fileSize);
    msg.request = O_DATA;
    msg.feedback = 0;
    msg.additional = 0;
    free(fileBuf);

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending file to server, errno = %d\n", errno);

    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        return -1;
    }
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }

    if(saveIntoDir(dirname, msg.additional) == -1) {
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    if(msg.additional==0) {
        SYSCALL_RETURN(readMessage, readMessage(fd_socket, &msg),
                     "ERROR OpenFile - Sending message to server, errno = %d\n", errno);

        if (msg.feedback != SUCCESS) {
            errno = msg.additional;
            freeMessageContent(&msg);
            return -1;
        }
        freeMessageContent(&msg);
    }
    return 0;
}

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    int scRes;
    message msg;
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }

    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_APPND;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);

    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }

    emptyMessage(&msg);
    msg.size = size;
    msg.content = buf;
    msg.request = O_DATA;
    msg.feedback = 0;
    msg.additional = 0;
    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    if(msg.feedback!=SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    /* Salvo i file se devono essere salvati */
    if(saveIntoDir(dirname, msg.additional) == -1) return -1;
    freeMessageContent(&msg);
    return 0;
}

int lockFile(const char* pathname){
    int scRes;
    message msg;
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_LOCK;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    /* Mutua esclusione ottenuta */
    freeMessageContent(&msg);
    return 0;
}

int unlockFile(const char* pathname){
    int scRes;
    message msg;
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_UNLOCK;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    /* Mutua esclusione rilasciata */
    freeMessageContent(&msg);
    return 0;
}

int closeFile(const char* pathname){
    int scRes;
    message msg;
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_CLOSE;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    /* File chiuso */
    return 0;
}

int removeFile(const char* pathname){
    int scRes;
    message msg;
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    memset(&msg, 0, sizeof(message));
    msg.size = strlen(pathname) + 1;
    msg.content = malloc(msg.size);
    strncpy(msg.content, pathname, msg.size);
    msg.request = O_DEL;
    msg.feedback = 0;
    msg.additional = 0;

    SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1){
        freeMessageContent(&msg);
        return -1;
    }
    freeMessageContent(&msg);
    SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                 "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
    if(scRes==-1) return -1;
    if(msg.feedback != SUCCESS){
        errno = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    /* File cancellato */
    freeMessageContent(&msg);
    return 0;
}

int saveIntoDir(const char* dir, int numOfFiles){
    int i;
    int scRes = 0;
    int errors = 0;
    void* path;
    message msg;
    int save = (dir==NULL)? 0 : 1;
    if(numOfFiles==0) return 0;
    memset(&msg, 0, sizeof(message));
    msg.additional=save;
    msg.feedback=SUCCESS;
    writeMessage(fd_socket, &msg);
    for(i = 0; i<numOfFiles; i++){
        /* Get PATH and PATHLEN */
        SYSCALL_ASSIGN(readMessage, scRes, readMessage(fd_socket, &msg),
                     "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
        if(scRes==-1){
            freeMessageContent(&msg);
            errno = EBADMSG;
            return -1;
        }
        /* IF ANY ERROR LET THE SERVER KNOW */
        if(msg.request!=O_SEND){
            freeMessageContent(&msg);
            msg.feedback = ERROR;
            msg.additional = EBADMSG;
            SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                         "ERROR OpenFile - Sending feedback error to server, errno = %d\n", errno);
            errno = EBADMSG;
            return -1;
        }
        path = malloc(msg.size+1);
        if(path == NULL){
            errno = ENOMEM;
            return -1;
        }
        strncpy(path, msg.content, msg.size+1);
        freeMessageContent(&msg);

        /* SUCCESS SEND ME THE FILE */
        msg.feedback = SUCCESS;
        SYSCALL_ASSIGN(writeMessage, scRes, writeMessage(fd_socket, &msg),
                     "ERROR OpenFile - Sending feedback error to server, errno = %d\n", errno);
        if(scRes==-1){
            return -1;
        }
        /* FILE RECEIVED */
        SYSCALL_EXIT(readMessage, scRes, readMessage(fd_socket, &msg),
                     "ERROR OpenFile - Sending message to server, errno = %d\n", errno);
        if(scRes==-1){
            return -1;
        }
        if(msg.feedback!=SUCCESS || msg.request!=O_DATA){
            freeMessageContent(&msg);
            free(path);
            errno = msg.additional;
            return -1;
        }
        if(save){
            if(storeFile(path, dir, msg.size, msg.content)==-1){
                errors = 1;
            }
        }
        freeMessageContent(&msg);
        free(path);

        /* if(fileBuf!=NULL) free(fileBuf); */
    }

    readMessage(fd_socket, &msg);
    if(msg.feedback!=SUCCESS){
        errors = msg.additional;
        freeMessageContent(&msg);
        return -1;
    }
    if(errors){
        freeMessageContent(&msg);
        errno = EIO;
        return -1;
    }
    freeMessageContent(&msg);
    return 0;
}

int storeFile(char* pathname, const char* saveDir, size_t size, void** buf){
    int clientPathDim = strlen(saveDir) + strlen(pathname) + 1;
    char* ClientPath = malloc(clientPathDim);
    FILE* fil3;
    int errors = 0;
    char* fileDir = ClientPath;
    if(ClientPath == NULL) return -1;
    sprintf(ClientPath, "%s%s", saveDir, pathname);
    fileDir = basename(fileDir);

    if(mkpath(ClientPath, S_IRWXU)==-1){
        return -1;
    }

    fil3 = fopen(ClientPath, "wb");
    if(fil3 == NULL){
        free(ClientPath);
        return -1;
    }

    if(buf){
        if(fwrite(buf, 1, size, fil3) < size){
            errors = 1;
        }
    }
    free(ClientPath);
    if (errors || (fclose(fil3) != 0))  {
        return -1;
    }
    return 0;
}

