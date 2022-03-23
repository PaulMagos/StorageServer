/*
 Created by paul on 07/01/22.
*/

#ifndef STORAGESERVER_API_H
#define STORAGESERVER_API_H

#include "utils.h"
#include "conn.h"

/**
 *   @func  openConnection
 *   @param sockname -> Server socket
 *   @param msec -> time to wait for reattempts
 *   @param abstime -> Max time to wait
 *   @effects -> opens connection with the server
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int openConnection(const char* sockname, int msec, const struct timespec abstime);
/**
 *   @func  closeConnection
 *   @param sockname -> Server socket
 *   @effects -> Close connection with the server
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int closeConnection(const char* sockname);
/**
 *   @func  openFile
 *   @param pathname -> file to open
 *   @param flags -> Open flags
 *   @effects -> opens a file on the server with "mode" specified in flags (Flags = { O_LOCK, O_CREAT, O_CREAT_LOCK })
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int openFile(const char* pathname, int flags);
/**
 *   @func  readFile
 *   @param pathname -> file to read from the server
 *   @param buf -> buf for the file content
 *   @param size -> size for the file size
 *   @effects -> read a file from the server and puts the file content into buffer and the size of the file in size
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int readFile(const char* pathname, void** buf, size_t* size);
/**
 *   @func  readNFiles
 *   @param N -> number of files to read from the server
 *   @param dirname -> if not null dir in which func saves the files
 *   @effects -> read n<=N files from the server and saves them in dirname with a path made from the files path
 *   @return -> returns -1 if there's a failure, n>=0 otherwise
 */
int readNFiles(int N, const char* dirname);
/**
 *   @func  writeFile
 *   @param pathname -> file to write on the server
 *   @param dirname -> if not null dir in which func saves the expelled files
 *   @effects -> writes a file on the server and saves the expelled ones if >0 in dirname with a path made from the files path
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int writeFile(const char* pathname, const char* dirname);
/**
 *   @func  appendToFile
 *   @param pathname -> file to update on the server
 *   @param buf -> file content to update on the server
 *   @param size -> file size to update on the server
 *   @param dirname -> if not null dir in which func saves the expelled files
 *   @effects -> updates a file on the server and saves the expelled ones if >0 in dirname with a path made from the files path
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);
/**
 *   @func  lockFile
 *   @param pathname -> file to lock on the server
 *   @effects -> locks a file on the server
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int lockFile(const char* pathname);
/**
 *   @func  unlockFile
 *   @param pathname -> file to unlock on the server
 *   @effects -> unlocks a file on the server
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int unlockFile(const char* pathname);
/**
 *   @func  closeFile
 *   @param pathname -> file to close on the server
 *   @effects -> closes a file on the server
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int closeFile(const char* pathname);
/**
 *   @func  removeFile
 *   @param pathname -> file to delete on the server
 *   @effects -> deletes a file on the server
 *   @return -> returns -1 if there's a failure, 0 otherwise
 */
int removeFile(const char* pathname);

#endif //STORAGESERVER_API_H
