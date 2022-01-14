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
bool getCmdList(List* opList, int argc, char* argv[]);

// Operation description for the client
static void usage(bool fFlag, bool pFlag);

void commandHandler(List* commandList);



int main(int argc,char* argv[]){


    // Command list creation and parsing
    List commandList = createList();

    if(getCmdList(&commandList, argc, argv))  commandHandler(&commandList);
    else(exit(1));

    // Free Command List
    deleteList(&commandList);
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

bool getCmdList(List* opList, int argc, char* argv[]){
    bool pFlag = false;
    bool fFlag = false;
    char option;

    while((option = (char)getopt(argc, argv, "hpf:w:W:D:r:R::d:t:l:u:c:")) != -1){
        if(optarg) {
            if(optarg[0]=='-'){
                fprintf(stderr, "%c command needs at least one parameter\n", option);
                return false;
            }
        }
        switch (option) {
            case 'h': {
                usage(fFlag, pFlag);
                break;
            }
            case 'p': {
                if(pFlag) {
                    fprintf(stderr, "ERROR - standard output already required, require it once\n");
                    return false;
                } else (pFlag = true);
                pushBottom(&(*opList), "p", optarg);
                break;
            }
            case 'f': {
                if(fFlag){
                    fprintf(stderr, "ERROR - socket can be set only once\n");
                    return false;
                }else (fFlag = true);
                pushBottom(&(*opList), "f", optarg);
                break;
            }
            case ':': {
                fprintf(stderr, "%c command needs at least one parameter\n", optopt);
                return false;
            }
            case '?': {
                fprintf(stderr, "Run with -h to see command list\n");
                return false;
            }
            default:{
                pushBottom(&(*opList), toOpt(option),optarg);
                break;
            };
        }

    }
    return true;
}

void commandHandler(List* commandList){
    char* socket = NULL;
    char* workingDir = NULL;
    char* expelledDir = "/dev/null";
    bool monOutput = false;


    char* command;
    char* argument;
    while ( pullTop(&(*commandList), &command, &argument) == 0){
        switch (command[0]) {
            case 'p':{
                printf("Stampa attivata\n");
                monOutput = !monOutput;
                msSleep()
                break;
            }
            case 'f':{
                printf("ciao '%c' %d\n", command[0],(*commandList)->len);
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
            case 't':{
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

