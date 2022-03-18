#if !defined(CONN_H)
#define CONN_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#define SOCKNAME     "../../tmp/cs_socket"

typedef enum{
    O_PEN = 0,
    O_CREAT = 1,
    O_LOCK = 2,
    O_CREAT_LOCK = 3,
    O_UNLOCK = 4,
    O_WRITE = 8,
    O_READ = 16,
    O_APPND = 32,
    O_DEL = 64,
    O_CLOSE = 128,
    O_DATA = 256,
    O_SEND = 512,
    O_READN = 1024,
    O_NULL = 2048,
} fileFlags;

typedef enum {
    SUCCESS = 1,
    ERROR = 2,
} fileResponse;

typedef struct{
    size_t size;
    void* content;
    int additional;
    fileFlags request;
    fileResponse feedback;
} message;

static inline char* requestToString(fileFlags request){
    switch (request) {
        case O_PEN: return "OPEN";
        case O_CREAT: return "CREATE";
        case O_LOCK: return "LOCK";
        case O_CREAT_LOCK: return "CREATE AND LOCK";
        case O_UNLOCK: return "UNLOCK";
        case O_WRITE: return "WRITE";
        case O_READ: return "READ";
        case O_APPND: return "APPEND";
        case O_DEL: return "DELETE";
        case O_CLOSE: return "CLOSE";
        case O_DATA: return "DATA";
        case O_SEND: return "SEND";
        case O_READN: return "READ N FILES";
        case O_NULL: return "NULL";
    }
    return "ERROR";
}

// Lezione 9 laboratorio, funzione di scrittura e lettura per evitare che rimangano dati sul buffer non gestiti
/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
        if ((r=read((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;   // EOF
        left -= r;
        bufptr  += r;
    }
    return size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return 1;
}


/**  Legge da fd in message1
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura ritorna 0
 *   \retval  size se la lettura termina con successo
 */
static inline int readMessage(int fd, message* message1){
    if(fd == -1 || message1 == NULL){
        errno = EINVAL;
        return -1;
    }
    printf("PREREAD\n");


    if(read(fd, message1, sizeof(message)) != sizeof(message)){
        return -1;
    }

    printf("POSTREAD\n");

    message1->content = malloc(message1->size + 1);
    memset(message1->content, 0, message1->size+1);
    int readed;
    if((readed = readn(fd, message1->content, message1->size)) == -1){
        fprintf(stderr, "Reading from %d fd", fd);
    }
    printf("READN: %d %d %s %d %s\n", fd, (int )message1->size, requestToString(message1->request), (int)message1->feedback, strerror(message1->additional));

    return readed;
}
/**  scrive a fd message1
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
static inline int writeMessage(int fd, message* message1){
    if(fd == -1 || message1 == NULL){
        errno = EINVAL;
        return -1;
    }
    printf("PREWRITE\n");

    if(write(fd, message1, sizeof(message)) != sizeof(message)){
        return -1;
    }
    printf("POSTWRITE\n");

    int written = 0;
    if(message1->size>0) {
        if ((written = writen(fd, message1->content, message1->size)) == -1) {
            fprintf(stderr, "Writing on %d fd", fd);
            message1->size = 0;
            free(message1->content);
        }
    }

    printf("WRITEN: %d %d %s %d %s\n", fd, (int )message1->size, requestToString(message1->request), (int)message1->feedback, strerror(message1->additional));

    return written;
}

/**  Valori default message1
 *
 */
static inline void emptyMessage(message* message1){
    message1->feedback = 0;
    message1->additional = 0;
}

/**  Libero content
 *
 */
static inline void freeMessageContent(message* message1){
    message1->size = 0;
    free(message1->content);
}
/**  Libero memoria
 *
 */
static inline void freeMessage(message* message1){
    if(message1->content!=NULL) free(message1->content);
    free(message1);
}


#endif /* CONN_H */
