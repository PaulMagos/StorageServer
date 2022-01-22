//
// Created by paul on 16/01/22.
//
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/socket.h>
#include "../../headers/api.h"
#include "../../headers/conn.h"
#include "../../headers/utils.h"

// Socket name
char* sName;
// Socket fd
int fd_socket;


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
    struct timespec diff = {.tv_sec = time1.tv_sec - time0.tv_sec, .tv_nsec =
    time1.tv_nsec - time0.tv_nsec};
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1000000000;
        diff.tv_sec--;
    }
    return diff;
}

int openConnection(const char* sockname, int msec, const struct timespec abstime){
    if(!sockname || msec<0){
        // INVALID ARGUMENTS
        errno = EINVAL;
        return -1;
    }

    // SYS CALL RESPONSE ( "Re-Used" for other functions when needed )
    int scRes = -1;
    int connRes;
    // Socket address
    struct sockaddr_un socketAdd;
    // Time variables
    struct timespec currenTime, iniTime;

    memset(&socketAdd, '0', sizeof(socketAdd));

    SYSCALL_EXIT(lengthCheck, scRes, lengthCheck(sockname), "ERROR - socket name too long, errno = %d\n", errno);
    strncpy(socketAdd.sun_path, sockname, UNIX_PATH_MAX);
    socketAdd.sun_family = AF_UNIX;

    SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &iniTime),
                 "ERROR - initializing time, errno = %d\n", errno);
    SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &currenTime),
                 "ERROR - getting current time, errno = %d\n", errno);
    SYSCALL_EXIT(socket, fd_socket, socket(AF_UNIX, SOCK_STREAM, 0),
                 "ERROR - socket connection failed, errno = %d\n", errno);
    while ((diff_timespec( abstime, diff_timespec(currenTime, iniTime))).tv_sec
        && (connRes = connect(fd_socket, (struct sockaddr*)&socketAdd, sizeof(socketAdd))) == -1){
        msleep(msec);
        SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &currenTime),
                     "ERROR - getting current time, errno = %d\n", errno);
        fprintf(stderr, "Trying connection... %d\n", (int)(currenTime.tv_sec-iniTime.tv_sec+1));
    }
    if(connRes==-1){
        fd_socket = -1;
        errno = ETIMEDOUT;
        return -1;
    }
    strcpy(sName, sockname);
    return 0;
}

int closeConnection(const char* sockname){
    if(!sockname){
        errno = EINVAL;
        return -1;
    }
    // if it's not the socket we are connected to, gives an error
    if(strcpy(sName, sockname)!=0){
        errno = ENOTCONN;
        return -1;
    }
    sName = NULL;
    int scRes;
    SYSCALL_EXIT(close, scRes, close(fd_socket), "ERROR - closing connection to %s, errno = %d\n", sockname, errno);
    fd_socket = -1;
    return 0;
}

int openFile(const char* pathname, int flags){

    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    // Variabile per testare le SC
    int scRes;

    // Invio al server il tipo di operazione richiesta
    operation op = of;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR OpenFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path, path e i flags al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR OpenFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR OpenFile - Sending path to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &flags, sizeof(int)),
                 "ERROR OpenFile - Sending flags to server, errno = %d\n", errno);
    // Dichiaro successivamente le altre variabili per evitare che
    // una chiamata che non va a buon fine blocchi il programma e rimangano inutilizzate
    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR OpenFile - Sending file to server, errno = %d\n", EBADMSG);
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }
    // File aperto correttamente
    return 0;
}

int readFile(const char* pathname, void** buf, size_t* size){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    // Invio al server il tipo di operazione richiesta
    operation op = r;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR ReadFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR ReadFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR ReadFile - Sending path to server, errno = %d\n", errno);

    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR ReadFile - Receiving file from server, errno = %d\n", EBADMSG);
    errno = 0;
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }

    // Se sono arrivato qua la comunicazione è andata a buon fine, devo leggere i dati e metterli nel buffer
    size_t fileDim;
    SYSCALL_EXIT(readn, scRes, readn(fd_socket, &fileDim, sizeof(int)),
                 "ERROR ReadFile - Receiving file from server [no_file], errno = %d\n", EBADMSG);
    void* buffer = NULL;
    if(!fileDim){
        buffer = malloc(fileDim);
        if(!buffer){
            return -1;
        }
        SYSCALL_EXIT(readn, scRes, readn(fd_socket, buffer, fileDim),
                     "ERROR ReadFile - Receiving file from server, errno = %d", EBADMSG);
        if(scRes == 0){
            // ERRORE comunicazione
            errno = EBADMSG;
            free(buffer);
            return -1;
        }
    }
    // Dimensione file nella size e stream ricevuto nel buffer
    *size = fileDim;
    *buf = buffer;
    return 0;
}

int readNFiles(int N, const char* dirname){
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = rn;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR ReadNFiles - Sending operation to server, errno = %d\n", errno);

    return 0;
}
/*
 *
 *
 *
 *
 * */

int writeFile(const char* pathname, const char* dirname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = wr;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR WriteFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR WriteFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR WriteFile - Sending path to server, errno = %d\n", errno);
    return 0;
}
/*
 *
 *
 *
 *
 * */

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = ap;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR WriteFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR WriteFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR WriteFile - Sending path to server, errno = %d\n", errno);
    return 0;
}
/*
 *
 *
 *
 *
 * */

int lockFile(const char* pathname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = lk;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR LockFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR LockFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR LockFile - Sending path to server, errno = %d\n", errno);
    // Dichiaro successivamente le altre variabili per evitare che
    // una chiamata che non va a buon fine blocchi il programma e rimangano inutilizzate
    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR LockFile - UnLocking file from server, errno = %d\n", EBADMSG);
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }
    // Mutua esclusione ottenuta
    return 0;
}

int unlockFile(const char* pathname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = unlk;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR UnLockFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR UnLockFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR UnLockFile - Sending path to server, errno = %d\n", errno);

    // Dichiaro successivamente le altre variabili per evitare che
    // una chiamata che non va a buon fine blocchi il programma e rimangano inutilizzate
    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR UnLockFile - UnLocking file from server, errno = %d\n", EBADMSG);
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }
    // Mutua esclusione rilasciata
    return 0;
}

int closeFile(const char* pathname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = cl;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR CloseFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR CloseFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR CloseFile - Sending path to server, errno = %d\n", errno);
    // Dichiaro successivamente le altre variabili per evitare che
    // una chiamata che non va a buon fine blocchi il programma e rimangano inutilizzate
    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR CloseFile - Closing file on server, errno = %d\n", EBADMSG);
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }
    // File chiuso
    return 0;
}

int removeFile(const char* pathname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }
    int scRes;
    operation op = del;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR RemoveFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR RemoveFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR RemoveFile - Sending path to server, errno = %d\n", errno);
    // Dichiaro successivamente le altre variabili per evitare che
    // una chiamata che non va a buon fine blocchi il programma e rimangano inutilizzate
    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR RemoveFile - Removing file from server, errno = %d\n", EBADMSG);
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }
    // File cancellato
    return 0;
}
