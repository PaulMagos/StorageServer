#!/bin/bash
MEMCHECK="valgrind --leak-check=full"
EXTERNTERMINAL="x-terminal-emulator -e ./scripts/newTerm"
I=1
stdOut="-p"


if(($1 == 1))
then
  I=1
  if [ -n "$2" ]
  then
    EXTERNTERMINAL=""
    stdOut=""
  fi
fi
if(($1 == 2))
then
  I=2
  MEMCHECK=""
  if [ -n "$2" ]
  then
    EXTERNTERMINAL=""
  fi
fi

TIME="-t 200"
CLIENT=./bin/client
SERVER=./bin/server
SOCKET=./tmp/cs_socket
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
${EXTERNTERMINAL} ${CLIENT} -f ${SOCKET} ${TIME} ${stdOut} -W ${WRITEFILESDIR}/file1.txt,${WRITEFILESDIR}/file2.txt -r ${WRITEFILESDIR}/file1.txt -c ${WRITEFILESDIR}/file2.txt &
echo
echo "AVVIO CLIENT 2 TESTO -w CON FILE ILLIMITATI E -D PER I FILE ESPULSI"
${EXTERNTERMINAL} ${CLIENT} -f ${SOCKET} ${TIME} ${stdOut} -w ${WRITEFILESDIR} -D ${EXPELLED} &
echo
echo "AVVIO CLIENT 3 TESTO -w CON 4 FILE AL MASSIMO, E -R CON -d PER SALVARE I FILE IN ${SAVEDIR}"
${EXTERNTERMINAL} ${CLIENT} -f ${SOCKET} ${TIME} ${stdOut} -w ${WRITEFILESDIR},4 -R -d ${SAVEDIR} &
echo
echo "AVVIO CLIENT 3 TESTO -l E -u DI 2 FILE, E -R"
${EXTERNTERMINAL} ${CLIENT} -f ${SOCKET} ${TIME} ${stdOut} -l ${WRITEFILESDIR}/file1.txt,${WRITEFILESDIR}/file2.txt -u ${WRITEFILESDIR}/file1.txt,${WRITEFILESDIR}/file2.txt -R &
(sleep 2; kill -1 ${SERVER_PID})

echo