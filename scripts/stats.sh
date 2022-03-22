#!/bin/bash
latest[1]=$1
COUNT=1

if [ -z "$1" ]
then
  COUNT=3
  DR=./log/test
  unset -v latest
  for i in {1..3}
  do
    log=""
    for file in $DR$i/*; do
      [[ $file -nt $log ]] && log=($file)
    done
    latest+=($log)
  done
fi

for TFile in ${latest[@]};
do
echo $TFile " STATS"
echo "----------------------------------------"
awk '/OPEN File/ {cnt++} END {printf ("OPENS: %20d\n",cnt)}' "${TFile}"
awk '/CLOSE File/ {cnt++} END {printf ("CLOSE: %20d\n",cnt)}' "${TFile}"
awk '/LOCK File/ {cnt++} END {printf ("LOCKS: %20d\n",cnt)}' "${TFile}"
awk '/READ File/ {cnt++} END {printf ("READS: %20d\n",cnt)}' "${TFile}"
awk '/WRITE File/ {cnt++} END {printf ("WRITES: %19d\n",cnt)}' "${TFile}"
awk '/UNLOCK File/ {cnt++} END {printf ("UNLOCKS: %18d\n",cnt)}' "${TFile}"
awk '/OPENLOCK File/ {cnt++} END {printf ("OPENLOCKS: %16d\n",cnt)}' "${TFile}"
awk -F ' ' '$3$4$5$6$7=="Maxnumberoffilessaved:" {printf ("MAX FILES: %16d\n",$8)}' "${TFile}"
awk -F ' ' '$3$4$5$6$7=="Maxbytesoffilessaved:" {su+=sprintf("%.5f", $10/1024^2)} END {printf ("MAX BYTES: %16.5f MB\n", su)}' "${TFile}"
awk -F ' ' '$3$4=="Maxclient:" {printf("MAX CLIENTS: %14d\n",$5)}' "${TFile}"
awk '/OPEN failed - No such/ {cnt++} END {printf ("OPEN FAILED: %14d\n",cnt)}' "${TFile}"
awk -F ' ' '$5 ~ /READ/ {sum+=$7; num+=1} END {printf ("AVG READ BYTES: %11.5f B\n", sum/num)}' "${TFile}"
awk -F ' ' '$5 ~ /READ/ {sum+=$7; num+=1} END {printf ("AVG WRITE BYTES: %10.5f B\n", sum/num)}' "${TFile}"
awk '/REPLACEALG/ {cnt++} END {printf ("REPLACEALG TIMES: %9d\n",cnt)}' "${TFile}"
awk '/CREATE AND LOCK failed - File exists/ {cnt++} END {printf ("CREATELOCK FAILED: %8d\n",cnt)}' "${TFile}"
echo "----------------------------------------"
awk -F ' ' '$3=="[Thread" && ( $6=="File" || $5=="REPLACEALG" ) {J=substr($4, 1, length($4)-2); su[J]+=1; stt[J, $5]+=1; if(J>max) max=J; if($5=="READ"){reads[J]+=$7}; if($5=="WRITE"){writes[J]+=$7}} END {for(x=0; x<=max; x++) {printf ("Thread %d op: %14d\n", x, su[x]); for(OP in stt) {split(OP, ind, SUBSEP); if(ind[1]==x) {printf("\t%15s: %d\n", ind[2], stt[x,ind[2]]);}}; {printf("\t%14s %d B\n\t%14s %d B\n", "READBYTES:", reads[x], "WRITEBYTES:", writes[x])}}}' "${TFile}"
echo "----------------------------------------"
echo

done
