//
// Created by paul on 12/01/22.
//

#ifndef STORAGESERVER_UTILS_H
#define STORAGESERVER_UTILS_H

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <getopt.h>
#include <dirent.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/un.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR   512
#endif

#define DIM_MSG 2048
#define MAX_DIM_LEN 1024
#define UNIX_PATH_MAX 108 /* man 7 unix */
#define KB 1024
#define MB 1048576
#define GB 1073741824

static const char* sizes[] = {"GB", "MB", "KB", "B"};


# define tSpecCmp(a, b, CMP)                                                  \
   (((a).tv_sec == (b).tv_sec) ?                                             \
   ((a).tv_nsec CMP (b).tv_nsec) :                                          \
   ((a).tv_sec CMP (b).tv_sec))

#define MAX(a,b) (((a)>(b))?(a):(b))

// ES 5 LEZIONI
#define SYSCALL_EXIT(name, r, sc, str, ...)	    \
    if ((r=sc) == -1) {				            \
	int errno_copy = errno;			            \
        if(strcmp(str, "")!=0 &&                \
        strcmp(str, " ")!=0){                   \
            perror(#name);                      \
            print_error(str, __VA_ARGS__);      \
        }                                       \
	exit(errno_copy);			                \
    }                                           \

#define SYSCALL_EXITFREE(name, toFree,r, sc, str, ...)	    \
    if ((r=sc) == -1) {				            \
	int errno_copy = errno;			            \
        if(strcmp(str, "")!=0 &&                \
        strcmp(str, " ")!=0){                   \
            perror(#name);                      \
            print_error(str, __VA_ARGS__);      \
        }                                       \
    free(toFree),                               \
	exit(errno_copy);			                \
    }                                           \

#define SYSCALL_RETURN(name, sc, str, ...)	    \
    if (sc == -1) {				                \
        if(strcmp(str, "")!=0 &&                \
        strcmp(str, " ")!=0){                   \
            perror(#name);                      \
            print_error(str, __VA_ARGS__);      \
        }                                       \
	return -1;			                        \
    }                                           \

#define SYSCALL_RET(name, sc, str, ...)	        \
    if (sc == -1) {				                \
        if(strcmp(str, "")!=0 &&                \
        strcmp(str, " ")!=0){                   \
            perror(#name);                      \
            print_error(str, __VA_ARGS__);      \
        }                                       \
	return;			                            \
    }                                           \

#define SYSCALL_ASSIGN(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				            \
    }                                           \

#define SYSCALL_BREAK(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				            \
        if(strcmp(str, "")!=0 &&                \
        strcmp(str, " ")!=0){                   \
            perror(#name);                      \
            print_error(str, __VA_ARGS__);      \
        }                                       \
    break;                                      \
    }                                           \
/**
* \brief Procedura di utilita' per la stampa degli errori
*
*/
static inline void print_error(const char * str, ...) {
    const char err[]="ERROR: ";
    va_list argp;
    char * p=(char *)malloc(strlen(str)+strlen(err)+EXTRA_LEN_PRINT_ERROR);
    if (!p) {
        perror("malloc");
        fprintf(stderr,"FATAL ERROR nella funzione 'print_error'\n");
        return;
    }
    strcpy(p,err);
    strcpy(p+strlen(err), str);
    va_start(argp, str);
    vfprintf(stderr, p, argp);
    va_end(argp);
    free(p);
}
// ES 5 LEZIONI

int nanosleep(const struct timespec *req, struct timespec *rem);

// msleep(): Sleep for the requested number of milliseconds.
// StackOverflow ->
// https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
static inline int msleep(long msec){
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

// Returns the char of the int str in ascii, 0 if it's not an alphabet letter
static inline int intIsChar(int str){
    if((str > 96 && str < 123) || (str > 64 && str < 91)) return str;
    else return 0;
}

// Returns a string with the char you give in str
static inline long StringToLong(char* str) {
    long result;

    if (str == NULL) {
        errno = EINVAL;
        return -1;
    }
    result = strtol(str, NULL, 10);
    if (result == 0) {
        errno = EINVAL;
        return INT_MAX;
    }

    if (result == LONG_MAX || result == LONG_MIN) {
        errno = ERANGE;
        return -1;
    }
    return result;
}


static inline int mkpath(char* file_path, mode_t mode) {
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

static inline char* calculateSize(size_t size){
    char* result = (char*) malloc(sizeof(char) * 20);
    float max = GB;
    for (int i = 0; i < (int)(sizeof(sizes)/sizeof(*sizes)); i++) {
        if(size < max) {
            max = max/1024;
            continue;
        }
        sprintf(result, "%.2f %s", ((float)size)/max, sizes[i]);
        return result;
    }
    strcpy(result, "0 B");
    return result;
}
#endif //STORAGESERVER_UTILS_H