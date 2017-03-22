ILP =		golem.o lgg.o progol.o refine.o clause_diff.o

CC = 		gcc

CFLAGS = 	-g -I. -I../interp

BINARIES =	../interp/interp.a ilp.a

LDFLAGS =	-lm

ilp:		$(BINARIES) version.o main.o
		$(CC) -o prolog_ilp version.o main.o $(ILP) $(BINARIES) $(LDFLAGS)

ilp.a:		$(ILP)
		ar -rc ilp.a $(ILP)
		ranlib ilp.a

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

install:	ilp
		cp prolog_ilp $(HOME)/bin

clean:
		rm -f prolog_ilp *.o *.a *~ core

# DEPENDENCIES

$(ILP):		../interp/prolog.h

