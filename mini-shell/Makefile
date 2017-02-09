# Alvin Ung
# November 29, 2016
# CS 352
# Assignment 6
#

#	$Id: Makefile,v 1.10 2016/12/02 15:47:40 unga2 Exp $

CC = gcc
CFLAGS = -g -Wall

SRCS = msh.c arg_parse.c builtin.c expand.c strmode.c
OUTP = $(SRCS:.c=.o)
EXE = msh

all: $(OUTP)
	 $(CC) $(CFLAGS) -o $(EXE) $(OUTP)

clean: 
		@echo "Removing all made files"
		@rm -f $(OUTP)
		@rm -f $(EXE)


# Dependency List
msh.o arg_parse.o builtin.o expand.o strmode.o: proto.h global.h
