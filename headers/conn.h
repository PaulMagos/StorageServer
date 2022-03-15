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
    of, // open
    wr, // write
    r, // read file
    rn, // read nFiles
    lk, // lock file
    unlk, // unlock file
    del, // delete file
    cl, // close file
    ap, // append to file
} operation;

typedef enum{
    O_CREAT = 1,
    O_LOCK = 2,
    O_UNLOCK = 4,
    O_WRITE = 8,
    O_READ = 16,
} fileFlags;

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
        left    -= r;
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

/** Sends operation intention to fd
 *
 *   \retval -1   error (errno settato)
 *   \retval  0   if success
 */
int sendOp(int fd, operation op){
    if(writen((long)fd, &op,sizeof(operation)) == -1){
        fprintf(stderr,"ERROR: Send operation - Sending operation to %d, errno = %d\n", fd, errno);
        errno = EBADMSG;
        return -1;
    }
    return 0;
}

/** Reads the operation intention from fd
 *
 *   \retval -1   error (errno settato)
 *   \retval  0   if client disconnected
 *   \retval  readen size if success
 */
int readOp(int fd, operation* op){
    int res = 0;
    if((res = readn((long)fd, &(*op),sizeof(operation))) == -1){
        if(writen((long)fd, &errno, sizeof(int)) == -1){
            fprintf(stderr,"ERROR: Read operation - Send error to %d, errno = %d\n", fd, errno);
        }
        fprintf(stderr,"ERROR: Read operation - Read operation from %d, errno = %d\n", fd, errno);
        errno = EBADMSG;
        return -1;
    } else if(res == 0){
        // Client disconnected
        return 0;
    }
    return res;
}

/** Reads Data from fd and saves them in len and buffer
 *
 *   \retval -1   error (errno settato)
 *   \retval  0   if success
 */
int readDataLength(int fd, int* len, void* buffer){
    if(readn((long)fd, &len, sizeof(int)) == -1) {
        fprintf(stderr, "ERROR - Reading length from %d, errno = %d", fd, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    buffer = malloc(*len);
    if(!(buffer)) {
        errno = ENOMEM;
        return -1;
    }
    if(len>0){
        if( readn((long)fd, &(buffer), *len) == -1) {
            fprintf(stderr, "ERROR - Reading content from %d, errno = %d", fd, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }
    return 0;
}

/** Sends data to the server, from len and buffer
 *
 *   \retval -1   error (errno settato)
 *   \retval  0   if success
 */
int writeDataLength(int fd, int* len, void* buffer){
    if(!(buffer) || *len < 0) {
        errno = EINVAL;
        return -1;
    }
    if(writen((long)fd, &len, sizeof(int)) == -1) {
        fprintf(stderr, "ERROR - Sending length to %d, errno = %d", fd, EBADMSG);
        errno = EBADMSG;
        return -1;
    }
    if(len!=0){
        if(writen((long)fd, &(buffer), *len) == -1) {
            fprintf(stderr, "ERROR - Sending content to %d, errno = %d", fd, EBADMSG);
            errno = EBADMSG;
            return -1;
        }
    }
    return 0;
}

#endif /* CONN_H */
