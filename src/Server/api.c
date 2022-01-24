//
// Created by paul magos on 16/01/22.
//
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "../../headers/api.h"
#include "../../headers/conn.h"
#include "../../headers/utils.h"

// Socket name
char* sName;
// Socket fd
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
    struct timespec diff = {.tv_sec = time1.tv_sec - time0.tv_sec, .tv_nsec =
    time1.tv_nsec - time0.tv_nsec};
    if (diff.tv_nsec < 0) {
        diff.tv_nsec += 1000000000;
        diff.tv_sec--;
    }
    return diff;
}

int saveIntoDir(const char* dir, int numOfFiles);
int storeFile(char* pathname, const char* saveDir, size_t size, void* buf);
int mkpath(char* file_path, mode_t mode);

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
                 "ERROR OpenFile - Sending file to server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
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


    // Verifico stato richiesta
    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR ReadFile - Receiving file from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
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
    // Comunico al server l'operazione e il numero di file
    operation op = rn;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR ReadNFiles - Sending operation to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &N, sizeof(int )),
                 "ERROR ReadNFiles - Sending N to the server, errno = %d", errno);

    int readed;
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR ReadNFiles - Sending operation and N to server, errno = %d\n", errno);
    // EBADMSG indica un fallimento nella comunicazione
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    if(scRes != 0){
        errno = scRes;
        return -1;
    }


    // Verifico quanti file vengono effettivamente letti (se sul server sono presenti n < N file?)
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR ReadNFiles - Retrieving files from server, errno = %d\n", errno);
    // EBADMSG indica un fallimento nella comunicazione
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    // Salvo i file se devono essere salvati, li leggo altrimenti
    if(saveIntoDir(dirname, readed) == -1) return -1;
    return 0;
}

int writeFile(const char* pathname, const char* dirname){
    if (pathname == NULL){
        errno = EINVAL;
        return -1;
    }
    if(fd_socket == -1){
        errno = ENOTCONN;
        return -1;
    }

    // LINUX/Posix stat library for retrieving a path info,
    // I could've used standard library by seeking to the end of the file and back,
    // but I've preferred this method
    // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    struct stat fileDet;
    if(stat(pathname, &fileDet) == -1){
        errno = EINVAL;
        return -1;
    }
    size_t fileSize = fileDet.st_size;

    // Alloco un buffer della dimensione del file e verifico che sia andata a buon fine l'allocazione
    void* fileBuf = malloc(sizeof(fileSize));
    if(fileBuf == NULL) {
        errno = ENOMEM;
        return -1;
    }

    // Tento l'apertura del file in lettura binaria e ne verifico l'esito
    FILE* fil3;
    if((fil3 = fopen(pathname, "rb")) == NULL){
        return -1;
    }
    // Leggo il file nel buffer e verifico la dimensione dell'entità letta dalla "fread"
    if(fread(fileBuf, 1, fileSize, fil3) < fileSize){
        free(fileBuf);
        errno = EIO;
        fclose(fil3);
        return -1;
    }

    if(fclose(fil3) != 0 ) return -1;

    // Invio operazione al server, path e lunghezza del path
    // Se non va a buon fine libero il buffer del file e ritorno -1
    operation op = wr;
    int length = strlen(pathname) + 1;
    if(writen(fd_socket, &op,sizeof(op)) == -1  ||
        writen(fd_socket, &length, sizeof(int)) == -1 ||
        writen(fd_socket, (void*) pathname, length) == -1){
        free(fileBuf);
        return -1;
    }

    // Verifico che il server sia pronto per ricevere il file
    int res;
    int readed = readn(fd_socket, &res, sizeof(int));
    if(readed < 1){
        free(fileBuf);
        if(readed == 0) errno = EBADMSG;
        return -1;
    }
    if(res!=0){
        free(fileBuf);
        errno = res;
        return -1;
    }

    // Invio dimensione del file e file
    if(writen(fd_socket, &fileSize, sizeof(int)) == -1){
        free(fileBuf);
        return -1;
    }
    if(fileSize>0){
        if(writen(fd_socket, &fileBuf, fileSize) == -1){
            free(fileBuf);
            return -1;
        }
    }
    free(fileBuf);

    // Verifico esito dell'operazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &res, sizeof(int)),
                 "ERROR WriteFile - Writing file from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    if(res!=0){
        errno = res; // Errore che il server ci passa
        return -1;
    }

    // Controllo quanti file sono stati espulsi; contro-intuitivamente la risposta non sta in readed, ma in res
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &res, sizeof(int)),
                 "ERROR WriteFile Reading num of expelled files from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }

    if(saveIntoDir(dirname, readed) == -1) return -1;
    return 0;
}


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
    // Invio al server il tipo di operazione richiesta
    operation op = ap;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &op,sizeof(op)),
                 "ERROR AppendToFile - Sending operation to server, errno = %d\n", errno);

    // Invio dimensione del path e path al server
    int length = strlen(pathname) + 1;
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &length, sizeof(int)),
                 "ERROR AppendToFile - Sending path size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, (void*) pathname, length),
                 "ERROR AppendToFile - Sending path to server, errno = %d\n", errno);

    int readed;
    // EBADMSG indica un fallimento nella comunicazione
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR AppendToFile - Retrieving status from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }

    // Se sono arrivato qua significa che posso effettuare la append sul file.
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, &size, sizeof(size_t)),
                 "ERRORE AppendToFile - Sending file size to server, errno = %d\n", errno);
    SYSCALL_EXIT(writen, scRes, writen(fd_socket, buf, size),
                 "ERRORE AppendToFile - Sending file to server, errno = %d\n", errno);
    // Verifico che tutto sia stato ricevuto correttamente
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR AppendToFile - Retrieving status from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }

    // Verifico quanti file vengono espulsi
    SYSCALL_EXIT(readn, readed, readn(fd_socket, &scRes, sizeof(int)),
                 "ERROR ReadNFiles - Retrieving files from server, errno = %d\n", errno);
    // EBADMSG indica un fallimento nella comunicazione
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    // Salvo i file se devono essere salvati
    if(saveIntoDir(dirname, readed) == -1) return -1;
    return 0;
}

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
                 "ERROR LockFile - UnLocking file from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
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
                 "ERROR UnLockFile - UnLocking file from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
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
                 "ERROR CloseFile - Closing file on server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
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
                 "ERROR RemoveFile - Removing file from server, errno = %d\n", errno);
    if(readed == 0){
        errno = EBADMSG;
        return -1;
    }
    if(scRes!=0){
        errno = scRes; // Errore che il server ci passa
        return -1;
    }
    // File cancellato
    return 0;
}

int saveIntoDir(const char* dir, int numOfFiles){
    bool errors = false;
    bool save = (dir==NULL)? false : true;
    int len, readed;
    void* path;
    size_t fileDim;
    for(int i = 0; i<numOfFiles; i++){

        SYSCALL_EXIT(readn, readed, readn(fd_socket, &len, sizeof(int)),
                     "ERROR - Saving files from server, errno = %d\n", errno);
        // EBADMSG indica un fallimento nella comunicazione
        if(readed == 0){
            errno = EBADMSG;
            return -1;
        }

        path = malloc(len);
        if(path == NULL){
            errno = ENOMEM;
            return -1;
        }

        // Leggo path
        readed = readn(fd_socket, &path, len);
        if(readed < 1){
            free(path);
            if(readed == 0) errno = EBADMSG;
            return -1;
        }

        // Leggo size del file
        readed = readn(fd_socket, &fileDim, sizeof(size_t));
        if(readed < 1){
            free(path);
            if(readed == 0) errno = EBADMSG;
            return -1;
        }

        void* fileBuf = NULL;
        if(fileDim != 0){
            fileBuf = malloc(fileDim);
            if(fileBuf == NULL) {
                free(path);
                return -1;
            }

            readed = readn(fd_socket, &fileBuf, fileDim);
            if(readed < 1){
                free(fileBuf);
                free(path);
                if(readed == 0) errno = EBADMSG;
                return -1;
            }
        }
        if(save){
            if(storeFile(path, dir, fileDim, fileBuf)==-1){
                errors = true;
            }
        }
        free(path);
        if(fileBuf!=NULL) free(fileBuf);
    }
    if(errors){
        errors = EIO;
        return -1;
    }
    return 0;
}

int storeFile(char* pathname, const char* saveDir, size_t size, void* buf){

    int clientPathDim = strlen(saveDir) + strlen(pathname) + 1;
    char* ClientPath = malloc(clientPathDim);
    if(ClientPath == NULL) return -1;
    snprintf(ClientPath, PATH_MAX, "%s/%s", saveDir, pathname);
    char* fileDir = basename(ClientPath);

    if(mkpath(fileDir, S_IRWXU)==-1){
        return -1;
    }

    FILE* fil3 = fopen(ClientPath, "wb");
    if(fil3 == NULL){
        free(ClientPath);
        free(fileDir);
        return -1;
    }

    bool errors = false;
    if(buf){
        if(fwrite(buf, 1, size, fil3) < size){
            errors = true;
        }
    }
    free(ClientPath);
    if (errors || (fclose(fil3) != 0))  {
        return -1;
    }
    return 0;
}

int mkpath(char* file_path, mode_t mode) {
    if(file_path == NULL) {
        errno = EINVAL;
        return -1;
    }
    for (char* p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
        *p = '\0';
        if (mkdir(file_path, mode) == -1) {
            if (errno != EEXIST) {
                *p = '/';
                return -1;
            }
        }
        *p = '/';
    }
    return 0;
}