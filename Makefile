#
# Makefile for smsh
#  as of 2022-03-29
#

CC = cc -Wall  -g


OBJS = smsh.o splitline2.o process.o varlib.o controlflow.o builtin.o \
		flexstr2.o

smsh: $(OBJS)
	$(CC) -o smsh $(OBJS)

builtin.o: builtin.c smsh.h varlib.h builtin.h 
	$(CC) -c builtin.c

controlflow.o: controlflow.c smsh.h process.h 
	$(CC) -c controlflow.c

flexstr2.o: flexstr2.c flexstr2.h splitline2.h 
	$(CC) -c flexstr2.c

process.o: process.c smsh.h builtin.h varlib.h controlflow.h process.h 
	$(CC) -c process.c

smsh.o: smsh.c smsh.h splitline2.h varlib.h process.h 
	$(CC) -c smsh.c

splitline2.o: splitline2.c splitline2.h smsh.h flexstr2.h 
	$(CC) -c splitline2.c

varlib.o: varlib.c varlib.h 
	$(CC) -c varlib.c

clean:
	rm -f *.o smsh
