#!/bin/bash
SERVER=./bin/server

echo "AVVIO SERVER CON VALGRIND"
valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all -s ${SERVER} ./files/test1/config1.txt ./log &
SERVER_PID=$!

