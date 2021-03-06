#!/bin/bash
MEMCHECK="valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s"

I=1
TIME="-t 200"
CLIENT=./bin/client
SERVER=./bin/server
SOCKET=./tmp/cs_socket
TERM=./scripts/newTerm
FORLDER=./files/test${I}
SAVEDIR=${FORLDER}/read
EXPELLED=${FORLDER}/expelled
WRITEFILESDIR=${FORLDER}/write


echo
echo "AVVIO SERVER"
${MEMCHECK} ${SERVER} ./configs/config${I}.txt ./log/test${I} &
SERVER_PID=$!
sleep 2
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
echo "AVVIO CLIENT 3 TESTO -l E -u DI 2 FILE, E -R"
x-terminal-emulator -e ${TERM} ${MEMCHECK} ${CLIENT} -f ${SOCKET} ${TIME} -p -l ${WRITEFILESDIR}/file1.txt,${WRITEFILESDIR}/file2.txt -u ${WRITEFILESDIR}/file1.txt,${WRITEFILESDIR}/file2.txt -R &
(kill -1 ${SERVER_PID})
