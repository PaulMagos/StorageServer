SHELL = /bin/sh
CC = gcc
INCLUDES = -I ./headers
CFLAGS += -std=c99 -Wall -Werror -Wextra -pedantic -g $(INCLUDES)


CLIENTOUTPUT = ./bin/client
SERVEROUTPUT = ./bin/server
CLIENTDIR = ./src/Client
SERVERDIR = ./src/Server
DATASTRUCTURES = ./src/DataStructures
APIDIR = ./src/Api
OBJS = $(DATAOBJ) $(CLIENTOBJ) $(SERVEROBJ) $(APIOBJ) $(MULTIOBJ) $(LOGOBJ)

TARGETS = $(CLIENTOUTPUT) $(SERVEROUTPUT)
all : genTexts $(TARGETS)
	make clean

# ---------------------------- DATA STRUCTURES ----------------------------- #
DATASRC = $(wildcard $(DATASTRUCTURES)/*.c)
DATAOBJ = $(patsubst $(DATASTRUCTURES)/%.c, $(DATASTRUCTURES)/%.o, $(DATASRC))

$(DATASTRUCTURES)/%.o: $(DATASTRUCTURES)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --------------------------------- CLIENT --------------------------------- #
CLIENTSRC = $(wildcard $(CLIENTDIR)/*.c)
CLIENTOBJ = $(patsubst $(CLIENTDIR)/%.c, $(CLIENTDIR)/%.o, $(CLIENTSRC))
APISRC = $(APIDIR)/api.c
APIOBJ = $(APIDIR)/api.o

$(CLIENTOUTPUT): $(CLIENTOBJ) $(DATAOBJ) $(APIOBJ)
	$(CC) $(CLIENTOBJ) $(DATAOBJ) $(APIOBJ) -o $@

$(CLIENTDIR)/%.o: $(CLIENTDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(APIOBJ): $(APISRC)
	$(CC) $(CFLAGS) -c $< -o $@

# --------------------------------- SERVER --------------------------------- #
SERVERSRC = $(wildcard $(SERVERDIR)/*.c)
MULTISRC = $(wildcard $(SERVERDIR)/multithread/*.c)
LOGSRC = $(wildcard $(SERVERDIR)/utility/*.c)
SERVEROBJ = $(patsubst $(SERVERDIR)/%.c, $(SERVERDIR)/%.o, $(SERVERSRC))
MULTIOBJ = $(patsubst $(SERVERDIR)/multithread/%.c, $(SERVERDIR)/multithread/%.o, $(MULTISRC))
LOGOBJ = $(patsubst $(SERVERDIR)/utility/%.c, $(SERVERDIR)/utility/%.o, $(LOGSRC))

$(SERVEROUTPUT): $(SERVEROBJ) $(DATAOBJ) $(MULTIOBJ) $(LOGOBJ)
	$(CC) $(SERVEROBJ) $(DATAOBJ) $(MULTIOBJ) $(LOGOBJ) -o $@

$(SERVERDIR)/%.o: $(SERVERDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVERDIR)/multithread/%.o: $(SERVERDIR)/multithread/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVERDIR)/utility/%.o: $(SERVERDIR)/utility/%.c
	$(CC) $(CFLAGS) -c $< -o $@



.PHONY: all clean
.SUFFIXES:
.SUFFIXES: .c .o .h

clean :
	rm -f $(OBJS)
cleanall :
	rm -f $(TARGETS) $(OBJS)
	make delTexts

test1:
	chmod +x ./scripts/test.sh &
	./scripts/test.sh 1

# IF x-terminal-emulator doesn't work
test1X:
	chmod +x ./scripts/test.sh &
	./scripts/test.sh 1 1

test2:
	chmod +x ./scripts/test.sh &
	./scripts/test.sh 2

# IF x-terminal-emulator doesn't work
test2X:
	chmod +x ./scripts/test.sh &
	./scripts/test.sh 2 1

test3:
	chmod +x ./scripts/test3.sh &
	./scripts/test3.sh

stats:
	chmod +x ./scripts/stats.sh &
	./scripts/stats.sh

# Genero cartella di log, bin, files e rispettivi file per ogni test, tmp per la socket,
# e new term per mantenere aperto un terminale (Se non esiste)
genTexts:
	chmod +x ./scripts/genTexts &
	./scripts/genTexts

# Elimino tutto quello che genero con genTexts (Se Esiste)
delTexts:
	chmod +x ./scripts/delTexts &
	./scripts/delTexts