#!/bin/bash
MEMCHECK="valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s"

CLIENT=./bin/client
SAVEDIR=./files/test1/read
EXPELLED=./files/test1/expelled
WRITEFILESDIR=./files/test1/write
TERM=./scripts/newTerm
SOCKET=./tmp/cs_socket
TIME="-t 200"
SERVER=./bin/server

echo
echo "AVVIO SERVER CON VALGRIND"
${MEMCHECK} ${SERVER} ./files/test1/config1.txt ./log &
SERVER_PID=$!
echo
echo "AVVIO CLIENT 1 TESTO -W CON DUE FILE -r PER IL PRIMO FILE E -c PER IL SECONDO"
x-terminal-emulator -e ${TERM} ${MEMCHECK} ${CLIENT} -f ${SOCKET} ${TIME} -p -W ${WRITEFILESDIR}/file1.txt,${WRITEFILESDIR}/file2.txt -r ${WRITEFILESDIR}/file1.txt -c ${WRITEFILESDIR}/file2.txt &
echo
echo "AVVIO CLIENT 2 TESTO -w CON FILE ILLIMITATI E -D PER I FILE ESPULSI"
x-terminal-emulator -e ${TERM} ${MEMCHECK} ${CLIENT} -f ${SOCKET} ${TIME} -p -w ${WRITEFILESDIR} -D ${EXPELLED} &
echo
echo "AVVIO CLIENT 3 TESTO -w CON 4 FILE AL MASSIMO, E -R CON -d PER SALVARE I FILE IN ${SAVEDIR}"
x-terminal-emulator -e ${TERM} ${MEMCHECK} ${CLIENT} -f ${SOCKET} ${TIME} -p -w ${WRITEFILESDIR},4 -R -d ${SAVEDIR} &
echo
echo "AVVIO CLIENT 3 TESTO -w CON 4 FILE AL MASSIMO, E -R CON -d PER SALVARE I FILE IN ${SAVEDIR}"
x-terminal-emulator -e ${TERM} ${MEMCHECK} ${CLIENT} -f ${SOCKET} ${TIME} -p -w ${WRITEFILESDIR},4 -R -d ${SAVEDIR} &
(sleep 5; kill -1 ${SERVER_PID})