//
// Created by paul on 12/01/22.
//

#ifndef STORAGESERVER_UTILS_H
#define STORAGESERVER_UTILS_H


#include <time.h>
#include <string.h>
#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if !defined(EXTRA_LEN_PRINT_ERROR)
#define EXTRA_LEN_PRINT_ERROR   512
#endif

#define DIM_MSG 2048
#define MAX_DIM_LEN 1024
#define UNIX_PATH_MAX 108 /* man 7 unix */

// ES 5 LEZIONI
#define SYSCALL_EXIT(name, r, sc, str, ...)	\
    if ((r=sc) == -1) {				\
	perror(#name);				\
	int errno_copy = errno;			\
	print_error(str, __VA_ARGS__);   \
	exit(errno_copy);			\
    }

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

// Returns the char of the int str in ascii, 0 if it's not an alphabet letter
char intToChar(int str);

// Returns a string with the char you give in str
char* toString(char str);
char toChar(char* str);

// msleep(): Sleep for the requested number of milliseconds.
// StackOverflow ->
// https://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds
int msleep(long msec);

long stringToLong(char* str);

#endif //STORAGESERVER_UTILS_H
