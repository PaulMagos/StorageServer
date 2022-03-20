#!/bin/bash

CLIENT=./bin/client
SAVEDIR=./files/test1/read
EXPELLED=./files/test1/expelled
WRITEFILESDIR=./files/test1/write
TERM=./scripts/newTerm
SOCKET=./tmp/cs_socket
TIME="-t 200"


echo
echo "AVVIO CLIENT 2 TESTO -w CON FILE ILLIMITATI E -D PER I FILE ESPULSI"
x-terminal-emulator -e ${TERM} ${CLIENT} -f ${SOCKET} ${TIME} -p -w ${WRITEFILESDIR} -D ${EXPELLED} &
echo
echo "AVVIO CLIENT 3 TESTO -w CON 4 FILE AL MASSIMO, E -R CON -d PER SALVARE I FILE IN ${SAVEDIR}"
x-terminal-emulator -e ${TERM} ${CLIENT} -f ${SOCKET} ${TIME} -p -w ${WRITEFILESDIR},4 -R -d ${SAVEDIR}&
echo
echo "AVVIO CLIENT 3 TESTO -w CON 4 FILE AL MASSIMO, E -R CON -d PER SALVARE I FILE IN ${SAVEDIR}"
x-terminal-emulator -e ${TERM} ${CLIENT} -f ${SOCKET} ${TIME} -p -w ${WRITEFILESDIR},4 -R -d ${SAVEDIR}&
sleep 2
