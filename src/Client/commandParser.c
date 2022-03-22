//
// Created by paul magos on 18/01/22.
//

#include "../../headers/client.h"


bool pFlag, fFlag, dFlag, DFlag;
long timeToSleep = 1;


int recWrite(char* dirname, char* expelledDir, long cnt, int indent);
char* charToString(char str);

void Helper(){
    fprintf(stdout,
        "-h \t\t\t help (commands description)\n"
               "-f filename  \t\t socket name (AF_UNIX socket)\n"
               "-w dirname[,n=0] \t sends n files from dirname, if n=0 it sends all the files it can;"
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
               "-c file1[,file2] \t list of file we want to remove from the server\n"
               "-p \t\t\t enables print on the standard output for each operation \n"
            );
}

int getCmdList(List* opList, int argc, char* argv[]){
    pFlag = false;
    fFlag = false;
    bool hFlag = false;
    char option;
    int test;
    char* timeArg = NULL;
    char* fArg = NULL;
    char* rNum = NULL;

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
                    fprintf(stdout, "SUCCESS - standard output activated\n");
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
                    SYSCALL_EXIT(pushBottom, test, pushBottom(&(*opList), charToString(option),optarg),
                                 "Error List Push, errno = %d\n", errno);
                    fArg = optarg;
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
            case 'R':{
                if(optind<argc) {
                    if (argv[optind][0] != '-') {
                        rNum = argv[optind];
                    }
                }
                SYSCALL_EXIT(pushBottom, test, pushBottom(&(*opList), charToString(option),rNum),
                             "Error List Push, errno = %d\n", errno);
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
                SYSCALL_EXIT(pushBottom, test, pushBottom(&(*opList), charToString(option),optarg),
                             "Error List Push, errno = %d\n", errno);
                break;
            };
        }

    }
    if(hFlag) Helper();
    if(timeArg!=NULL){
        SYSCALL_EXIT(StringToLong, timeToSleep, StringToLong(timeArg), (pFlag)?
                     "ERROR - Time Char '%s' to Long Conversion gone wrong, errno=%d\n":"", timeArg, errno);
        if(timeToSleep==INT_MAX) timeToSleep=0;
        if(pFlag) fprintf(stdout, "SUCCESS - time = %lu\n", timeToSleep);
    }
    if(fFlag){
        // Provo a collegarmi per 10 secondi
        struct timespec absTime = {10, 0};
        SYSCALL_EXIT(openConnection,
                     test,
                     openConnection(fArg,
                                    1000,
                                    absTime),
                     (pFlag)?"Connection error to socket %s, errno %d\n":"",
                     fArg,
                     errno);
        if(pFlag) fprintf(stdout, "SUCCESS - Connected to %s\n", fArg);
        msleep(timeToSleep);
    }
    if(search((*opList)->head, "D")){
        if(!(search((*opList)->head, "w") || search((*opList)->head, "W"))) {
            fprintf(stderr, (pFlag)?"ERROR - D command must be used with 'w' or 'W' commands\n":"");
            errno = EINVAL;
            return -1;
        }
        DFlag = true;
    } else DFlag = false;
    if(search((*opList)->head, "d")){
        if(!(search((*opList)->head, "r") || search((*opList)->head, "R"))) {
            fprintf(stderr, (pFlag)?"ERROR - d command must be used with 'r' or 'R' commands\n":"");
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
    long numOfFilesToWrite = INT_MAX;
    long numOfFilesToRead = INT_MAX;

    // Response for SYSCALL_EXIT
    int scRes = 0;

    // Command and argument for each element of opList
    char* command = NULL;
    void* argument = NULL;

    // Path details, to establish if it's a directory
    struct stat dir_Details;
    char* token; // For strtok_r
    char* rest; // For strtok_r
    char* path = NULL;
    char* temporary;

    signal(SIGPIPE, SIG_IGN);
    // Control if given arguments for expelled files, and readen files from server are directories
    if(DFlag) {
        getArg((*commandList)->head, "D", &expelledDir);
        if(expelledDir) {
            if(stat(expelledDir,&dir_Details)==-1){
                if(pFlag) fprintf(stderr, "DIR %s not exits, exit...\n", expelledDir);
                free(socket);
                return;
            }
            if(!S_ISDIR(dir_Details.st_mode))  expelledDir = NULL;
            else if(pFlag) {
                if(expelledDir[strlen(expelledDir)-1] == '/') expelledDir[strlen(expelledDir)-1] = '\0';
                fprintf(stdout, "'%s' -> Expelled Files DIR Set, exit...\n", expelledDir);
            }
        }
        memset(&dir_Details, 0, sizeof(dir_Details));
    }
    if(dFlag) {
        getArg((*commandList)->head, "d", &readDir);
        if (readDir) {
            if(stat(readDir,&dir_Details)==-1){
                if((pFlag)) fprintf(stderr, "DIR %s not exits\n", readDir);
                free(socket);
                return;
            }
            if (!S_ISDIR(dir_Details.st_mode)) readDir = NULL;
            else if (pFlag) {
                if(readDir[strlen(readDir)-1] == '/') readDir[strlen(readDir)-1] = '\0';
                fprintf(stdout, "'%s' -> Received Files DIR Set\n", readDir);
            }
        }
        memset(&dir_Details, 0, sizeof(dir_Details));
    }

    while ( pullTop(&(*commandList), &command, &argument) == 0){
        switch (*command) {
            case 'w':{
                if(command) free(command);
                token = strtok_r(argument, ",", &rest);
                if(stat(token, &dir_Details)==-1){
                    if(pFlag) fprintf(stderr, (pFlag) ? "DIR %s not exits, exit...\n":" ", token);
                    if(socket) free(socket);
                    if(argument) free(argument);
                    exit(errno);
                }
                if(S_ISDIR(dir_Details.st_mode)){
                    if((temporary = strtok_r(NULL, ",", &rest)) != NULL)
                        SYSCALL_EXIT(StringToLong, numOfFilesToWrite, StringToLong(temporary), (pFlag)?
                                     "Char '%s' to Long Conversion gone wrong, errno=%d\n":"", temporary, errno);
                    if(pFlag) fprintf(stdout, "Accessing Folder %s : \n", token);
                    scRes = recWrite(token, expelledDir, numOfFilesToWrite, 0);
                } else{
                    fprintf(stderr, "'%s' -> Not a valid DIR\n",token);
                }
                if(scRes == -1 || (errno == 9 || errno == EPIPE)){
                    if(socket) free(socket);
                    if(argument) free(argument);
                    exit(errno);
                }
                memset(&dir_Details, 0, sizeof(dir_Details));
                msleep(timeToSleep);
                break;
            }
            case 'W':{
                if(command) free(command);
                token = strtok_r(argument, ",", &rest);
                stat(token, &dir_Details);
                path = NULL;
                while (token) {
                    if((path = realpath(token, path)) == NULL)
                        fprintf(stderr, (pFlag) ? "'%s' -> File not exits\n": " ", token);
                    else{
                        if(!path || stat(path, &dir_Details)==-1){
                            token = strtok_r(NULL, ",", &rest);
                            continue;
                        }
                        if(S_ISDIR(dir_Details.st_mode)) {
                            fprintf(stderr, "'%s' -> This is a directory\n", token);
                            scRes = -1;
                            break;
                        }
                        SYSCALL_ASSIGN(openFile, scRes, openFile(path, O_CREAT_LOCK), (pFlag)?
                            "'%s' -> WRITE Open failed, errno = %d\n":"", token, errno);
                        if(scRes!=-1){
                            SYSCALL_ASSIGN(writeFile, scRes, writeFile(path, expelledDir), (pFlag)?
                                "'%s' -> WRITE Write failed, errno = %d\n":"", token, errno);
                            if(scRes!=-1) {
                                SYSCALL_ASSIGN(closeFile, scRes, closeFile(path), (pFlag) ?
                                                          "'%s' -> WRITE Close failed, errno = %d\n":"", token, errno);
                                if (pFlag && scRes != -1) fprintf(stdout, "%s -> Write Success\n", token);
                            }
                        }
                    }
                    token = strtok_r(NULL, ",", &rest);
                    if(scRes == -1){
                        if(errno!=17) {
                            //if(path) free(path);
                            if(argument) free(argument);
                            if(socket) free(socket);
                            exit(errno);
                        }
                    }
                }
                free(path);
                msleep(timeToSleep);
                break;
            }
            case 'r':{
                if(command) free(command);
                token = strtok_r(argument, ",", &rest);
                stat(token, &dir_Details);
                path = NULL;
                while (token) {
                    if ((path = realpath(token, path)) == NULL)
                        fprintf(stderr, (pFlag) ? "'%s' -> File not exits\n":" ", token);
                    else {
                        SYSCALL_BREAK(openFile, scRes, openFile(path, 0), (pFlag) ?
                            "'%s' -> READ Open failed, errno = %d\n" : "", token, errno);
                        void *buffer;
                        size_t size;
                        SYSCALL_BREAK(readFile, scRes, readFile(path, &buffer, &size), (pFlag) ?
                            "'%s' -> READ Read failed, errno = %d\n" : "", token, errno);
                        if (readDir != NULL){
                            char clientPath[UNIX_PATH_MAX];
                            memset(clientPath, 0, UNIX_PATH_MAX);
                            char* fileName = path;
                            sprintf(clientPath, "%s%s", readDir, fileName);
                            char* fileDir = clientPath;
                            fileDir = basename(fileDir);

                            if(mkpath(clientPath, S_IRWXU)==-1) continue;

                            FILE* clientFile = fopen(clientPath, "wb");
                            if(clientFile){
                                fwrite(buffer, 1, size, clientFile);
                                //fprintf(clientFile, "%s", buffer);
                            }
                            else fprintf(stderr, (pFlag)? "'%s' -> Saving failed\n":" ", token);
                            fclose(clientFile);
                        }
                        free(buffer);

                        SYSCALL_BREAK(closeFile, scRes, closeFile(path), (pFlag) ?
                            "'%s' -> READ Close failed, errno = %d\n" : "", token, errno);
                        if (pFlag) fprintf(stdout, "%s -> Read Success\n", token);
                    }
                    if(scRes == -1 || (errno == 9 || errno == EPIPE)){
                        if(path) free(path);
                        if(socket) free(socket);
                        if(argument) free(argument);
                        exit(errno);
                    }
                    token = strtok_r(NULL, ",", &rest);
                    msleep(timeToSleep);
                }
                free(path);
                break;
            }
            case 'R':{
                if(command) free(command);
                if(argument!=NULL) {
                    SYSCALL_BREAK(StringToLong, numOfFilesToRead, StringToLong(argument), (pFlag) ?
                                 "ERROR - ReadNF Char '%s' to Long Conversion gone wrong, errno=%d\n":"", argument, errno);
                }
                SYSCALL_BREAK(readNFiles, scRes, readNFiles(numOfFilesToRead, readDir), (pFlag) ?
                             "ReadN failed, errno = %d\n":"", errno);
                if(pFlag) fprintf(stdout, "Files Readed, saved in %s\n", readDir);
                if(scRes == -1 || numOfFilesToRead==-1 || (errno == 9 || errno == EPIPE)){
                    if(socket) free(socket);
                    if(argument) free(argument);
                    exit(errno);
                }
                msleep(timeToSleep);
                break;
            }
            case 'l':{
                if(command) free(command);
                token = strtok_r(argument, ",", &rest);
                path = NULL;
                while (token){
                    if(!(path = realpath(token, path))){
                        fprintf(stderr, (pFlag) ? "'%s' -> File not exits\n":"\n", token);
                    } else{
                        stat(path, &dir_Details);
                        if(S_ISREG(dir_Details.st_mode)){
                            SYSCALL_BREAK(openFile, scRes, openFile(path, 0), (pFlag) ?
                                        "'%s' -> LOCK Open failed, errno = %d\n" : "", token, errno);
                            SYSCALL_BREAK(lockFile, scRes, lockFile(path), (pFlag) ?
                                         "'%s' -> LOCK Lock failed, errno = %d\n":"", token, errno);
                            SYSCALL_BREAK(closeFile, scRes, closeFile(path), (pFlag) ?
                                        "'%s' -> LOCK Close failed, errno = %d\n" : "", token, errno);
                            if(pFlag) fprintf(stdout, "%s -> Lock Success\n", token);
                        }else{
                            fprintf(stderr, (pFlag)? "ERROR - '%s' not a file\n":" ", token);
                        }
                    }
                    token = strtok_r(NULL, ",", &rest);
                    msleep(timeToSleep);
                }
                if(scRes == -1 || (errno == 9 || errno == EPIPE)){
                    if(path) free(path);
                    if(socket) free(socket);
                    if(argument) free(argument);
                    exit(errno);
                }
                free(path);
                memset(&dir_Details, 0, sizeof(dir_Details));
                break;
            }
            case 'u':{
                if(command) free(command);
                token = strtok_r(argument, ",", &rest);
                path = NULL;
                while (token){
                    if(!(path = realpath(token, path))){
                        fprintf(stderr, (pFlag) ? "'%s' -> File not exits\n":" ", token);
                    } else{
                        stat(path, &dir_Details);
                        if(S_ISREG(dir_Details.st_mode)){
                            SYSCALL_BREAK(openFile, scRes, openFile(path, 0), (pFlag) ?
                                         "'%s' -> UNLOCK Open failed, errno = %d\n" : "", token, errno);
                            SYSCALL_BREAK(unlockFile, scRes, unlockFile(path), (pFlag) ?
                                         "'%s' -> UNLOCK Unlock failed, errno = %d":"", token, errno);
                            SYSCALL_BREAK(closeFile, scRes, closeFile(path), (pFlag) ?
                                        "'%s' -> UNLOCK Close failed, errno = %d\n" : "", token, errno);
                            if(pFlag) fprintf(stdout, "%s -> Unlock Success\n", token);
                        }else{
                            fprintf(stderr, (pFlag)? "ERROR - '%s' not a file\n":" ", token);
                        }
                    }
                    token = strtok_r(NULL, ",", &rest);
                    msleep(timeToSleep);
                }
                if(scRes == -1 || (errno == 9 || errno == EPIPE)){
                    if(path) free(path);
                    if(socket) free(socket);
                    if(argument) free(argument);
                    exit(errno);
                }
                free(path);
                memset(&dir_Details, 0, sizeof(dir_Details));
                break;
            }
            case 'c':{
                if(command) free(command);
                token = strtok_r(argument, ",", &rest);
                path = NULL;
                while (token){
                    if(!(path = realpath(token, path))){
                        fprintf(stderr, (pFlag) ? "'%s' -> File not exits\n":" ", token);
                    } else{
                        stat(path, &dir_Details);
                        if(S_ISREG(dir_Details.st_mode)){
                            SYSCALL_BREAK(removeFile, scRes, removeFile(path), (pFlag) ?
                                         "'%s' -> Delete failed, errno = %d\n":"", token, errno);
                            if(pFlag) fprintf(stdout, "%s -> Delete Success\n", token);
                        }else{
                            fprintf(stderr, (pFlag)? "'%s' -> Not a file\n":" ", token);
                        }
                    }
                    token = strtok_r(NULL, ",", &rest);
                    msleep(timeToSleep);
                }
                if(scRes == -1 || (errno == 9 || errno == EPIPE)){
                    if(path) free(path);
                    if(socket) free(socket);
                    if(argument) free(argument);
                    exit(errno);
                }
                free(path);
                memset(&dir_Details, 0, sizeof(dir_Details));
                break;
            }
            case 'D':{
                if(command) free(command);
                break;
            }
            case 'd':{
                if(command) free(command);
                break;
            }
            case 'f':{
                if(command) free(command);
                socket = malloc(strlen(argument)+1);
                strncpy(socket, argument, strlen(argument)+1);
                break;
            }
        }
        if(argument) free(argument);
        argument = NULL;
    }

    msleep(timeToSleep);
    SYSCALL_EXIT(closeConnection, scRes, closeConnection(socket), (pFlag) ? "ERROR closing connection to %s, errno = %d":"", socket, errno);
    free(socket);
    return;
}

int recWrite(char* dirname, char* expelledDir, long cnt, int indent){
    DIR* directory;
    struct dirent* element;
    long filesToWrite = cnt;
    int scRes = 0;
    if((directory = opendir(dirname))==NULL || filesToWrite == 0) return 0;
    char* path;
    errno = 0;
    // StackOverflow has something similar at
    // https://stackoverflow.com/questions/8436841/how-to-recursively-list-directories-in-c-on-linux/29402705
    while ((element = readdir(directory)) != NULL && filesToWrite > 0){
        char newPath[PATH_MAX];
        snprintf(newPath, sizeof(newPath), "%s%c%s", dirname, (dirname[strlen(dirname)-1] == '/')? 0:'/',element->d_name);
        switch(element->d_type){
            case DT_REG:{
                path = NULL;
                if((path = realpath(newPath, path)) == NULL) fprintf(stderr, (pFlag)? "ERROR - Opening File %s\n":" ", newPath);
                else{
                    SYSCALL_BREAK(openFile, scRes, openFile(path, O_CREAT_LOCK), (pFlag)?
                        "ERROR - REWRITE Couldn't send file %s to server, errno = %d\n": "", element->d_name, errno);
                    SYSCALL_BREAK(writeFile, scRes, writeFile(path, expelledDir), (pFlag)?
                        "ERROR - REWRITE Couldn't write file %s on server, errno = %d\n": "", element->d_name, errno);
                    SYSCALL_BREAK(closeFile, scRes, closeFile(path), (pFlag)?
                        "ERROR - REWRITE Couldn't close file %s on server, errno = %d\n": "", element->d_name, errno);
                    if(pFlag) printf("%*s- %s -> WRITE SUCCESS\n", indent, "", element->d_name);
                    filesToWrite--;
                }
                free(path);
                msleep(timeToSleep);
                break;
            }
            case DT_DIR: {
                if (strcmp(element->d_name, ".") == 0 || strcmp(element->d_name, "..") == 0) continue;
                if (pFlag) printf("%*s[%s]\n", indent, "", element->d_name);
                filesToWrite  = filesToWrite - recWrite(newPath, expelledDir, filesToWrite, indent + 2);
                break;
            }
            default:{
                break;
            }
        }
        if(scRes == -1) {
            free(path);
            if(errno == EPIPE){
                closedir(directory);
                return -1;
            }
        }
    }
    closedir(directory);
    if(scRes==-1){
        return -1;
    }
    return cnt - filesToWrite;
}

char* charToString(char str){
    switch (str) {
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




