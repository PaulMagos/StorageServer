//
// Created by paul on 16/01/22.
//
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include "../../headers/api.h"
#include "../../headers/utils.h"

const char* sName;

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
    // Socket fd
    int fd_socket;
    // Socket address
    struct sockaddr_un socketAdd;
    // Time variables
    struct timespec currenTime, iniTime;

    memset(&socketAdd, '0', sizeof(socketAdd));

    SYSCALL_EXIT(lengthCheck, scRes, lengthCheck(sockname), "ERROR - socket name too long, errno = %d\n", errno);
    strncpy(socketAdd.sun_path, sockname, UNIX_PATH_MAX);
    socketAdd.sun_family = AF_UNIX;

    SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &iniTime),
                 "ERROR - initializing time, errno = %d", errno);
    SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &currenTime),
                 "ERROR - getting current time, errno = %d", errno);
    SYSCALL_EXIT(socket, fd_socket, socket(AF_UNIX, SOCK_STREAM, 0),
                 "ERROR - socket connection failed, errno = %d", errno);
    while ((diff_timespec( abstime, diff_timespec(currenTime, iniTime))).tv_sec && (scRes = connect(fd_socket, (struct sockaddr*)&socketAdd, sizeof(socketAdd))) == -1){
        fprintf(stderr, "trying : %d, %f, - %d, %f\n", (int)diff_timespec(currenTime, iniTime).tv_sec,  (float)diff_timespec(currenTime, iniTime).tv_nsec, abstime.tv_sec, abstime.tv_nsec);
        msleep(msec);
        SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &currenTime),
                     "ERROR - getting current time, errno = %d", errno);
    }
    if(scRes==-1){
        fd_socket = -1;
        errno = ETIMEDOUT;
        return -1;
    }
    sName = sockname;
    return 0;
}
/*
 *
 *
 *
 *
 * */

int closeConnection(const char* sockname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

int openFile(const char* pathname, int flags){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int readFile(const char* pathname, void** buf, size_t* size){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int readNFiles(int N, const char* dirname){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int writeFile(const char* pathname, const char* dirname){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);
/*
 *
 *
 *
 *
 * */

int lockFile(const char* pathname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

int unlockFile(const char* pathname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

int closeFile(const char* pathname){
    return 0;
}
/*
 *
 *
 *
 *
 * */

int removeFile(const char* pathname){
    return -1;
}
/*
 *
 *
 *
 *
 * */

