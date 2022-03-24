#!/bin/bash
MEMCHECK="valgrind --leak-check=full"

I=3
TIME="-t 0"
CLIENT=./bin/client
SERVER=./bin/server
SOCKET=./tmp/cs_socket
FORLDER=./files/test${I}
SAVEDIR=${FORLDER}/read
EXPELLED=${FORLDER}/expelled
WRITEFILESDIR=${FORLDER}/write

echo
echo "AVVIO SERVER"
${SERVER} ./configs/config${I}.txt ./log/test${I} &
SERVER_PID=$!
THIS_PID=$$

sleep 2
endTime=$(($SECONDS+30))
SEC=$(($endTime-$SECONDS))
while (( $SEC>0 ))
do
  J=$(($RANDOM%10))+1
  # Scrivo 5 file dalla cartella J, J prende un valore da 1 a 10, quando arriva ad un valore > 10 utilizzo il modulo 10
  # per evitare che cerchi cartelle inesistenti
  ${CLIENT} -f ${SOCKET} "${TIME}" -w "${WRITEFILESDIR}"/dir${J} -D ${EXPELLED} &
  # Scrivo 5 file dalla cartella 2
  ${CLIENT} -f ${SOCKET} "${TIME}" -w ${WRITEFILESDIR}/dir2,5 &
  # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir3/file31.txt -r ${WRITEFILESDIR}/dir3/file31.txt -l ${WRITEFILESDIR}/dir3/file31.txt -u ${WRITEFILESDIR}/dir3/file31.txt -c ${WRITEFILESDIR}/dir3/file31.txt &
  # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir4/file41.txt -r ${WRITEFILESDIR}/dir4/file41.txt -l ${WRITEFILESDIR}/dir4/file41.txt -u ${WRITEFILESDIR}/dir4/file41.txt -c ${WRITEFILESDIR}/dir4/file41.txt &
  # Scrivo 5 file dalla cartella 5 E SETTO LA CARTELLA PER I FILE ESPULSI
  ${CLIENT} -f ${SOCKET} "${TIME}" -w ${WRITEFILESDIR}/dir5,5 -D ${EXPELLED} &
  # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir5/file51.txt -r ${WRITEFILESDIR}/dir5/file51.txt -l ${WRITEFILESDIR}/dir5/file51.txt -u ${WRITEFILESDIR}/dir5/file51.txt -c ${WRITEFILESDIR}/dir5/file51.txt &
   # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir6/file61.txt -r ${WRITEFILESDIR}/dir6/file61.txt -l ${WRITEFILESDIR}/dir6/file61.txt -u ${WRITEFILESDIR}/dir6/file61.txt -c ${WRITEFILESDIR}/dir6/file61.txt &
  # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir7/file72.txt -r ${WRITEFILESDIR}/dir7/file72.txt -l ${WRITEFILESDIR}/dir7/file72.txt -u ${WRITEFILESDIR}/dir7/file72.txt -c ${WRITEFILESDIR}/dir7/file72.txt &
  # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir9/file92.txt -r ${WRITEFILESDIR}/dir9/file92.txt -l ${WRITEFILESDIR}/dir9/file92.txt -u ${WRITEFILESDIR}/dir9/file92.txt -c ${WRITEFILESDIR}/dir9/file92.txt &
  # Scrivo 1 file lo leggo, richiedo la lock, rilascio la lock, lo cancello
  ${CLIENT} -f ${SOCKET} "${TIME}" -W ${WRITEFILESDIR}/dir10/file102.txt -r ${WRITEFILESDIR}/dir10/file102.txt -l ${WRITEFILESDIR}/dir10/file102.txt -u ${WRITEFILESDIR}/dir10/file102.txt -c ${WRITEFILESDIR}/dir10/file102.txt &
  # Richiedo la lettura di 10 file e setto la directory per salvarli
  ${CLIENT} -f ${SOCKET} "${TIME}" -R 10 -d ${SAVEDIR} &
  ${CLIENT} -f ${SOCKET} "${TIME}" -R 10 -d ${SAVEDIR} &
  ${CLIENT} -f ${SOCKET} "${TIME}" -R 10 -d ${SAVEDIR} &
  ${CLIENT} -f ${SOCKET} "${TIME}" -R 10 -d ${SAVEDIR}
  if (( $SEC > $endTime-$SECONDS ))
  then
    clear
    SEC=$(($endTime-$SECONDS))
    echo $SEC  # Stampo il tempo mancante
  fi
  sleep 1 # Necessario per evitare che terminino le risorse per la fork ( La READ è più veloce delle altre operazioni e crea troppi fork )
done

(kill -s SIGINT ${SERVER_PID}; kill ${THIS_PID})