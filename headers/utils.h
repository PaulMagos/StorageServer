//
// Created by paul on 12/01/22.
//

#ifndef STORAGESERVER_UTILS_H
#define STORAGESERVER_UTILS_H

#endif //STORAGESERVER_UTILS_H

#include <time.h>
#include <errno.h>

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