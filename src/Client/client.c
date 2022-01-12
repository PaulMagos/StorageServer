//
// Created by paul on 11/01/22.
//

#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "../../headers/api.h"
#include "../../headers/list.h"
#include "../../headers/utils.h"

// Operation description for the client
static void usage(bool fFlag, bool pFlag);

// Parse the commands given by execution of the client
List getCmdList(int argc, char** argv);


int main(int argc,char** argv){
    char* socket = NULL;
    char* workingDir = NULL;
    char* expelledDir = "/dev/null";
    List commandList = getCmdList(argc, argv);

    printList(commandList);

    deleteList(commandList);
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

List getCmdList(int argc, char** argv){
    bool pFlag = false;
    bool fFlag = false;
    char option;

    // Command list with arguments
    List opList =    createList();
    if(opList == NULL){
        return NULL;
    }

    while((option = (char)getopt(argc, argv, "hf:w:W:S:r:R::d:t:l:u:c:p"))){
        switch (option) {
            case 'h': {
                usage(fFlag, pFlag);
                break;
            }
            case 'p': {
                (pFlag)? fprintf(stderr, "ERROR - standard output already required") : (pFlag = true);
                break;
            }
            case 'f': {
                (fFlag)? fprintf(stderr, "ERROR - socket already set") : (fFlag = true);
            }
            case '?': {
                fprintf(stderr, "Run with -h to see command list");
                break;
            }
            case ':': {
                fprintf(stderr, "%c command needs at least one parameter", optopt);
                break;
            }
            default:{
                char* tmp = toChar(optopt);
                char* t = char* tmp;
                pushTop(opList, (const char*) t, optarg);
                break;
            };
        }
    }
    return opList;
}

