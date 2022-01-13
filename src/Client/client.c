//
// Created by paul on 11/01/22.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "../../headers/api.h"
#include "../../headers/list.h"
#include "../../headers/utils.h"


// Parse the commands given by execution of the client
void getCmdList(list** opList, int argc, char* argv[]);

// Operation description for the client
static void usage(bool fFlag, bool pFlag);



int main(int argc,char* argv[]){
    //char* socket = NULL;
    //char* workingDir = NULL;
    //char* expelledDir = "/dev/null";
    list* commandList = createList();
    getCmdList(&commandList, argc, argv);

    printList(commandList);

    //int t = deleteList(&commandList);
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

void getCmdList(list** opList, int argc, char* argv[]){
    bool pFlag = false;
    bool fFlag = false;
    char option;
    char* temp = malloc(2*sizeof (char));

    while((option = (char)getopt(argc, argv, "hpf:w:W:D:r:R::d:t:l:u:c:")) != -1){
        if(optarg[0]=='-') {
            fprintf(stderr, "%c command needs at least one parameter\n", option);
            exit(1);
        }
        switch (option) {
            case 'h': {
                usage(fFlag, pFlag);
                break;
            }
            case 'p': {
                (pFlag)? fprintf(stderr, "ERROR - standard output already required\n") : (pFlag = true);
                break;
            }
            case 'f': {
                (fFlag)? fprintf(stderr, "ERROR - socket already set\n") : (fFlag = true);
                pushTop(&(*opList), "f", optarg);
                break;
            }/*
            case 'w': {
                pushTop(&(*opList), "w", optarg);
                break;
            }
            case 'W': {
                pushTop(&(*opList), "W", optarg);
                break;
            }
            case 'D': {
                pushTop(&(*opList), "D", optarg);
                break;
            }
            case 'r': {
                pushTop(&(*opList), "r", optarg);
                break;
            }
            case 'R': {
                pushTop(&(*opList), "R", optarg);
                break;
            }
            case 'd': {
                pushTop(&(*opList), "d", optarg);
                break;
            }
            case 't': {
                pushTop(&(*opList), "t", optarg);
                break;
            }
            case 'l': {
                pushTop(&(*opList), "l", optarg);
                break;
            }
            case 'c': {
                pushTop(&(*opList), "c", optarg);
                break;
            }
            case 'u': {
                pushTop(&(*opList), "u", optarg);
                break;
            }*/
            case '?': {
                fprintf(stderr, "Run with -h to see command list\n");
                break;
            }
            case ':': {
                fprintf(stderr, "%c command needs at least one parameter\n", optopt);
                break;
            }
            default:{
                temp[0] = option;
                temp[1] = '\0';
                pushTop(&(*opList), temp,optarg);
                break;
            };
        }
    }
}
