//
// Created by paul magos on 18/01/22.
//
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "../../headers/api.h"
#include "../../headers/utils.h"
#include "../../headers/commandParser.h"

bool pFlag, fFlag, dFlag, DFlag;
long timeToSleep = 0;

static void usage(){
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
void recWrite(char* dirname, char* expelledDir, int cnt);
int getCmdList(List* opList, int argc, char* argv[]){
    pFlag = false;
    fFlag = false;
    bool hFlag = false;
    char option;
    int test;
    char* timeArg = NULL;

    while((option = (char)getopt(argc, argv, "hpf:w:W:D:r:R::d:t:l:u:c:")) != -1){
        if(optarg) {
            if(optarg[0]=='-'){
                fprintf(stderr, "%c command needs at least one parameter\n", option);
                errno = EINVAL;
                return -1;
            }
        }
        switch (option) {
            case 'p': {
                if(!pFlag){
                    fprintf(stderr, "SUCCESS - standard output activated\n");
                    (pFlag = true);
                } else{
                    fprintf(stderr, "ERROR - standard output already required, require it once\n");
                    errno = EINVAL;
                    return -1;
                }
                break;
            }
            case 'f': {
                if((fFlag)){
                    fprintf(stderr, "ERROR - socket can be set only once\n");
                    errno = EINVAL;
                    return -1;
                }else {
                    SYSCALL_EXIT(pushBottom, test, pushBottom(&(*opList), toString(option),optarg),
                                 "Error List Push, errno = %d\n", errno);
                    ((fFlag) = true);
                    break;
                }
            }
            case 'h': {
                hFlag = true;
                break;
            }
            case 't': {
                if(timeArg==NULL) timeArg = optarg;
                break;
            }
            case ':': {
                fprintf(stderr, "%c command needs at least one parameter\n", optopt);
                errno = EINVAL;
                return -1;
            }
            case '?': {
                fprintf(stderr, "Run with -h to see command list\n");
                errno = EINVAL;
                return -1;
            }
            default:{
                SYSCALL_EXIT(pushBottom, test, pushBottom(&(*opList), toString(option),optarg),
                             "Error List Push, errno = %d\n", errno);
                break;
            };
        }

    }
    if(hFlag) usage();
    if(timeArg!=NULL){
        SYSCALL_EXIT(stringToLong, timeToSleep, stringToLong(timeArg),
                     "Char '%s' to Long Conversion gone wrong, errno=%d\n", timeArg, errno);
        if(pFlag) fprintf(stderr, "SUCCESS - time = %lu\n", timeToSleep);
    }
    if(search((*opList)->head, "D")){
        if(!(search((*opList)->head, "w") || search((*opList)->head, "W"))) {
            fprintf(stderr, "ERROR - D command must be used with 'w' or 'W' commands\n");
            errno = EINVAL;
            return -1;
        }
        DFlag = true;
    } else DFlag = false;
    if(search((*opList)->head, "d")){
        if(!(search((*opList)->head, "r") || search((*opList)->head, "R"))) {
            fprintf(stderr, "ERROR - d command must be used with 'r' or 'R' commands\n");
            errno = EINVAL;
            return -1;
        }
        dFlag = true;
    } else dFlag = false;
    commandHandler(&(*opList));
    return 0;
}

void commandHandler(List* commandList){
    // Arguments for f, d, D, w, R
    char* socket = NULL;
    char* readDir = NULL;
    char* expelledDir = NULL;
    long numOfFilesToWrite = 0;
    //long numOfFilesToRead = 0;

    // Response for SYSCALL_EXIT
    int scRes;

    // Command and argument for each element of opList
    char* command;
    char* argument;

    // Path details, to establish if it's a directory
    struct stat dir_Details;
    char* temporary;

    // Control if given arguments for expelled files, and readen files from server are directories
    if(DFlag) {
        getArg((*commandList)->head, "D", &expelledDir);
        if(expelledDir) {
            stat(expelledDir,&dir_Details);
            if(!S_ISDIR(dir_Details.st_mode))  expelledDir = "dev/null";
            else if(pFlag) fprintf(stderr, "SUCCESS - Expelled Directory set : %s\n", expelledDir);
        }
        memset(&dir_Details, 0, sizeof(dir_Details));
    }
    if(dFlag) {
        getArg((*commandList)->head, "d", &readDir);
        if(readDir) {
            stat(readDir,&dir_Details);
            if(!S_ISDIR(dir_Details.st_mode))  readDir = "dev/null";
            else if(pFlag) fprintf(stderr, "SUCCESS - Received Files Directory set : %s\n", readDir);
        }
        memset(&dir_Details, 0, sizeof(dir_Details));
    }

    while ( pullTop(&(*commandList), &command, &argument) == 0){
        switch (toChar(command)) {
            case 'f':{
                if(fFlag) fFlag = !fFlag;
                else continue;
                socket = argument;
                struct timespec absTime;
                SYSCALL_EXIT(clock_gettime, scRes, clock_gettime(CLOCK_REALTIME, &absTime),
                             "Error during getTime, errno = %d\n", errno);
                absTime.tv_sec = absTime.tv_sec + 60;
                SYSCALL_EXIT(openConnection,
                             scRes,
                             openConnection(socket,
                                            (timeToSleep>0)? timeToSleep : 1000,
                                            absTime),
                             "Connection error to socket %s, errno %d\n",
                             argument,
                             errno);
                if(pFlag) fprintf(stderr, "SUCCESS - Connected to %s", socket);
                msleep(timeToSleep);
                break;
            }
            case 'w':{
                char* rest;
                char* dir = strtok_r(argument, ",", &rest);
                stat(dir, &dir_Details);
                if(S_ISDIR(dir_Details.st_mode)){
                    if((temporary = strtok_r(NULL, ",", &rest)) != NULL)
                        SYSCALL_EXIT(stringToLong, numOfFilesToWrite, stringToLong(temporary),
                                     (pFlag)? "Char '%s' to Long Conversion gone wrong, errno=%d\n" : "", temporary, errno);
                    recWrite(dir, expelledDir, numOfFilesToWrite);
                } else{
                    fprintf(stderr, "ERROR - writing files from directory '%s', not a valid directory\n",dir);
                }
                msleep(timeToSleep);
                break;
            }
            case 'W':{
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
            case 'D':{
                break;
            }
            case 'd':{
                break;
            }
        }
    }
    return;
}

void recWrite(char* dirname, char* expelledDir, int cnt){

}