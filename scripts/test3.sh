#!/bin/bash
MEMCHECK="valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s"

I=3
TIME="-t 0"
CLIENT=./bin/client
SERVER=./bin/server
TERM=./scripts/newTerm
SOCKET=./tmp/cs_socket
FORLDER=./files/test${I}
SAVEDIR=${FORLDER}/read
EXPELLED=${FORLDER}/expelled
WRITEFILESDIR=${FORLDER}/write
endTime=$(($SECONDS+30))


echo
echo "AVVIO SERVER"
${MEMCHECK} ${SERVER} ./configs/config${I}.txt ./log/test${I} &
SERVER_PID=$!
THIS_PID=$$

sleep 2

while (( $SECONDS < $endTime ))
do
  J=$(($((J + 1))%10))
  ${CLIENT} -f ${SOCKET} "${TIME}" -w "${WRITEFILESDIR}"/dir${J},5 -D ${EXPELLED} &
  ${CLIENT} -f ${SOCKET} "${TIME}" -w ${WRITEFILESDIR}/dir2,5 &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir3/file31.txt -r ${WRITEFILESDIR}/dir3/file31.txt -l ${WRITEFILESDIR}/dir3/file31.txt -u ${WRITEFILESDIR}/dir3/file31.txt -c ${WRITEFILESDIR}/dir3/file31.txt &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir4/file41.txt -r ${WRITEFILESDIR}/dir4/file41.txt -l ${WRITEFILESDIR}/dir4/file41.txt -u ${WRITEFILESDIR}/dir4/file41.txt -c ${WRITEFILESDIR}/dir4/file41.txt &
  ${CLIENT} -f ${SOCKET} "${TIME}" -w ${WRITEFILESDIR}/dir5,5 -D ${EXPELLED} &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir5/file51.txt -r ${WRITEFILESDIR}/dir5/file51.txt -l ${WRITEFILESDIR}/dir5/file51.txt -u ${WRITEFILESDIR}/dir5/file51.txt -c ${WRITEFILESDIR}/dir5/file51.txt &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir6/file61.txt -r ${WRITEFILESDIR}/dir6/file61.txt -l ${WRITEFILESDIR}/dir6/file61.txt -u ${WRITEFILESDIR}/dir6/file61.txt -c ${WRITEFILESDIR}/dir6/file61.txt &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir7/file72.txt -r ${WRITEFILESDIR}/dir7/file72.txt -l ${WRITEFILESDIR}/dir7/file72.txt -u ${WRITEFILESDIR}/dir7/file72.txt -c ${WRITEFILESDIR}/dir7/file72.txt &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir9/file92.txt -r ${WRITEFILESDIR}/dir9/file92.txt -l ${WRITEFILESDIR}/dir9/file92.txt -u ${WRITEFILESDIR}/dir9/file92.txt -c ${WRITEFILESDIR}/dir9/file92.txt &
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir10/file102.txt -r ${WRITEFILESDIR}/dir10/file102.txt -l ${WRITEFILESDIR}/dir10/file102.txt -u ${WRITEFILESDIR}/dir10/file102.txt -c ${WRITEFILESDIR}/dir10/file102.txt &
  sleep 1
done

(kill -s SIGINT ${SERVER_PID}; kill ${THIS_PID})

exit