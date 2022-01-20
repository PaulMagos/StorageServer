//
// Created by paul on 11/01/22.
//

#include "../headers/utils.h"
#include <errno.h>
#include <time.h>
#include <limits.h>



char intIsChar(int str){
    if((str > 96 && str < 123) || (str > 64 && str < 91)) return (char)str;
    else return 0;
}

char* toString(char str){
    switch (str) {
        case 'h': {
            return "h";
        }
        case 'p': {
            return "p";
        }
        case 'f': {
            return "f";
        }
        case 'w': {
            return "w";
        }
        case 'W': {
            return "W";
        }
        case 'D': {
            return "D";
        }
        case 'r': {
            return "r";
        }
        case 'R': {
            return "R";
        }
        case 'd': {
            return "d";
        }
        case 't': {
            return "t";
        }
        case 'l': {
            return "l";
        }
        case 'c': {
            return "c";
        }
        case 'u': {
            return "u";
        }
    }
    return 0;
}
char toChar(char* str){
    return intIsChar((int)str[0]);
}

int msleep(long msec)
{
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

long stringToLong(char* str) {
    long result;

    if (str == NULL) {
        errno = EINVAL;
        return -1;
    }
    result = strtol(str, NULL, 10);

    if (result == 0) {
        errno = EINVAL;
        return -1;
    }

    if (result == LONG_MAX || result == LONG_MIN) {
        errno = ERANGE;
        return -1;
    }
    return result;
}