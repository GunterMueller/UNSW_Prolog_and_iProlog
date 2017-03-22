GUI =		face.o fbrowser_cb.o generic.o instance_display.o \
		rdr_cb.o rdr_GUI.o string_io.o xforms_io.o xmessage.o
 
CC = 		gcc

CFLAGS = 	-g -I. -I../interp -I../KBS -I../Probot \
		-I/usr/X11R6/include/X11 -I/usr/include/X11\
		-D _GNU_SOURCE

LIBS =		-L/usr/X11R6/lib \
		-lm -lX11 -lXpm -lforms -lnetpbm		

BINARIES =	../interp/interp.a ../KBS/kbs.a ../Probot/probot.a gui.a

gui:		$(BINARIES) version.o main.o
		$(CC) -o prolog_gui version.o main.o $(GUI) $(BINARIES) $(LIBS)

gui.a:		$(GUI)
		ar -rc gui.a $(GUI)
		ranlib gui.a

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

install:	gui
		cp prolog_gui $(HOME)/bin

clean:
		rm -f prolog_gui *.o *.a *~ core

# DEPENDENCIES

$(PROBOT):	../interp/prolog.h

