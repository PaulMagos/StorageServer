#!/bin/bash
MEMCHECK="valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s"

CLIENT=./bin/client
SAVEDIR=./files/test2/read
EXPELLED=./files/test2/expelled
WRITEFILESDIR=./files/test2/write
TERM=./scripts/newTerm
SOCKET=./tmp/cs_socket
TIME="-t 200"
SERVER=./bin/server

