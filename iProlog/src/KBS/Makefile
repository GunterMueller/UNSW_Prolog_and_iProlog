KBS =		frame.o print_rule.o rdr.o rdr_fn.o rdr_plist.o
  
CC = 		gcc

CFLAGS = 	-g -I. -I../interp

BINARIES =	../interp/interp.a kbs.a

kbs: 		$(BINARIES) version.o main.o
		$(CC) -o prolog_kbs version.o main.o $(BINARIES) -lm

kbs.a:		$(KBS)
		ar -rc kbs.a $(KBS)
		ranlib kbs.a

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

install:	kbs
		cp prolog_kbs $(HOME)/bin

clean:
		rm -f prolog_kbs *.o *.a *~ core

# DEPENDENCIES

$(KBS):		../interp/prolog.h

