ML =		aq.o atms.o attr_info.o bp.o duce.o hash.o \
		heap.o id.o induct.o rt.o scw.o set.o

  
CC = 		gcc

CFLAGS = 	-g -I. -I../interp

BINARIES =	../interp/interp.a ml.a

LDFLAGS =	-lm

ml:		$(BINARIES) version.o main.o
		$(CC) -o prolog_ml version.o main.o $(ML) $(BINARIES) $(LDFLAGS)

ml.a:		$(ML)
		ar -rc ml.a $(ML)
		ranlib ml.a

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

install:	ml
		cp prolog_ml $(HOME)/bin

clean:
		rm -f prolog_ml *.o *.a *~ core

# DEPENDENCIES

$(KBS):		../interp/prolog.h

