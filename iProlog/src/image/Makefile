IMAGE =		bfl.o blob.o edge.o p_image.o p_pbm.o p_pgm.o p_ppm.o \
		skeleton.o trace.o
 
CC = 		gcc

CFLAGS = 	-g -I. -I../interp

LIBS =		-lm -lnetpbm

BINARIES =	../interp/interp.a image.a

image:		$(BINARIES) version.o main.o
		$(CC) -o prolog_im version.o main.o $(BINARIES) $(LIBS)

image.a:	$(IMAGE)
		ar -rc image.a $(IMAGE)
		ranlib image.a

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

install:	image
		cp prolog_im $(HOME)/bin

clean:
		rm -f prolog_im *.o *.a *~ core

# DEPENDENCIES

$(PROBOT):	../interp/prolog.h

