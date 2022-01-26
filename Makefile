SHELL = /bin/sh
CC = GCC
CFLAGS += -std=gnu99 -Wall -Werror -Wextra -pedantic -g
INCLUDES = -I ./headers

CLIENT = bin/client
SERVER = bin/server
OBJECTS = objs/

TARGETS =


.PHONY: all clean cleanall test
.SUFFIXES:
.SUFFIXES: .c .o .h


all : $(TARGETS)



clean :
	-rm -f $(TARGETS)
cleanall :
	-rm -f ./bin ./socket/* ./log