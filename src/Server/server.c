//
// Created by paul magos on 11/01/22.


#include "../../headers/server.h"

int main(int argc, char* argv[]){
    if(argc < 2) argv[1]= 0;
    int fd_skt;
    struct sockaddr_un sa;
    unlink("../../tmp/cs_socket");
    memset(&sa, '0', sizeof(sa));
    strncpy(sa.sun_path, "../../tmp/cs_socket", UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if((fd_skt=socket(AF_UNIX, SOCK_STREAM,0))==-1){
        perror("socket");
        exit(EXIT_FAILURE);//errore fatale
    }
    if(bind(fd_skt,(struct sockaddr *) &sa, sizeof(sa))==-1){
        perror("bind");
        exit(EXIT_FAILURE);//errore fatale
    }
    if(listen(fd_skt,1)==-1){
        perror("listen");
        exit(EXIT_FAILURE);//errore fatale
    }

    int client_fd;
    do{
        (client_fd = accept(fd_skt, NULL, NULL) );
        while (recv(client_fd,NULL,1, MSG_PEEK | MSG_DONTWAIT) != 0) {
            sleep(rand() % 2); // Sleep for a bit to avoid spam
            fflush(stdin);
            printf("I am alive: %d\n", client_fd);
        }
        break;
    } while (1);

}
