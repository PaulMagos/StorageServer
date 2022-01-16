//
// Created by paul on 11/01/22.
//

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <getopt.h>
#include "../../headers/api.h"
#include "../../headers/list.h"
#include "../../headers/utils.h"


// Parse the commands given by execution of the client
int getCmdList(List* opList, int argc, char* argv[], bool* pFlag, bool* fFlag, long* timeToSleep);

// Operation description for the client
static void usage(bool fFlag, bool pFlag);

void commandHandler(List* commandList, bool pFlag, bool fFlag, long* timeToSleep);



int main(int argc,char* argv[]){
    bool pflag, fFlag;
    int del=0;
    long timeToSleep = 0;

    // Command list creation
    List commandList;
    SYSCALL_EXIT(createList, del, createList(&commandList), "Error during list creation, errno = %d\n", errno);

    // Command list parsing
    if(getCmdList(&commandList, argc, argv, &pflag, &fFlag, &timeToSleep) == 0)
        commandHandler(&commandList, pflag, fFlag, &timeToSleep);
    else(exit(22));

    // Free Command List
    SYSCALL_EXIT(deleteList, del, deleteList(&commandList), "Errore during list free, errno = %d\n", errno);
    return 0;
}

static void usage(bool fFlag, bool pFlag){
    fprintf(stderr,"-h \t\t\t help (commands description)\n");
    fprintf(stderr,(!fFlag)? "-f filename  \t\t socket name (AF_UNIX socket)\n":"");
    fprintf(stderr,"-w dirname[,n=0] \t sends n files from dirname, if n=0 it sends all the files it can; "
                   "recursively access sub directories\n"
                   "-W file1[,file2] \t list of files to send to the server, detached by comma\n"
                   "-D dirname \t\t expelled files (from server) directory for client, it has to be used "
                   "with -w or -W, if not specified the files sent by the server will be ignored by the client\n"
                   "-r file1[,file2] \t list of files to read from the server, detached by comma\n"
                   "-R [n=0] \t\t read n files from the server, if n = 0 or not specified, it reads all "
                   "the files on the server\n"
                   "-d dirname \t\t directory name for storing the files readen from the server\n"
                   "-t time \t\t waiting time between queries to the server\n"
                   "-l file1[,file2] \t list of file on which we need mutual exclusion\n"
                   "-u file1[,file2] \t list of file on which we have to release mutual exclusion\n"
                   "-c file1[,file2] \t list of file we want to remove from the server\n");
    fprintf(stderr,(!pFlag)? "-p \t\t\t enables print on the standard output for each operation \n":"");
}

int getCmdList(List* opList, int argc, char* argv[],bool* pFlag,bool* fFlag, long* timeToSleep){
    *pFlag = false;
    *fFlag = false;
    char option;
    int test;

    while((option = (char)getopt(argc, argv, "hpf:w:W:D:r:R::d:t:l:u:c:")) != -1){
        if(optarg) {
            if(optarg[0]=='-'){
                fprintf(stderr, "%c command needs at least one parameter\n", option);
                return -1;
            }
        }
        switch (option) {
            case 'h': {
                usage(*fFlag, *pFlag);
                break;
            }
            case 'p': {
                if(*pFlag) {
                    fprintf(stderr, "ERROR - standard output already required, require it once\n");
                    return -1;
                } else {
                    fprintf(stderr, "SUCCESS - standard output activated\n");
                    (*pFlag = true);
                }
                break;
            }
            case 'f': {
                if((*fFlag)){
                    fprintf(stderr, "ERROR - socket can be set only once\n");
                    return -1;
                }else ((*fFlag) = true);
                pushBottom(&(*opList), "f", optarg);
                break;
            }
            case 't': {
                int tmp = 0;
                SYSCALL_EXIT(stringToLong, tmp, stringToLong(optarg),
                             (pFlag)? "Char '%s' to Long Conversion gone wrong, errno=%d\n" : "", optarg, errno);
                *timeToSleep = tmp;
                if(*pFlag) fprintf(stderr, "SUCCESS - time = %lu\n", *timeToSleep);
                break;
            }
            case ':': {
                fprintf(stderr, "%c command needs at least one parameter\n", optopt);
                return -1;
            }
            case '?': {
                fprintf(stderr, "Run with -h to see command list\n");
                return -1;
            }
            default:{
                SYSCALL_EXIT(pushBottom, test, pushBottom(&(*opList), toString(option),optarg),
                             "Error no List, errno = %d\n", errno);
                break;
            };
        }

    }
    return 0;
}

void commandHandler(List* commandList, bool pFlag, bool fFlag, long* timeToSleep){
    //char* socket = NULL;
    //char* workingDir = NULL;
    //char* expelledDir = "/dev/null";
    int scres;

    char* command;
    char* argument;


    while ( pullTop(&(*commandList), &command, &argument) == 0){
        switch (toChar(command)) {
            case 'f':{
                if(fFlag) fFlag = !fFlag;
                else continue;
                struct timespec absTime;
                SYSCALL_EXIT(clock_gettime, scres, clock_gettime(CLOCK_REALTIME, &absTime),
                             "Error during getTime, errno = %d\n", errno);
                absTime.tv_sec = absTime.tv_sec + 60;
                SYSCALL_EXIT(openConnection,
                             scres,
                             openConnection(argument,
                                            (*timeToSleep>0)? *timeToSleep : 1000,
                                            absTime),
                             "Connection error to socket %s, errno %d\n",
                             argument,
                             errno);
                if(pFlag) fprintf(stderr, "SUCCESS - Connected to %s", argument);
                msleep(*timeToSleep);
                break;
            }
            case 'w':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'W':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'D':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'r':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'R':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'd':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'l':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'u':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
            case 'c':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
                break;
            }
        }
    }
    return;
}

