PROBOT =	probot.o read_script.o rdr_chat.o
 
CC = 		gcc

CFLAGS = 	-g -I. -I../interp -I../KBS

BINARIES =	../interp/interp.a ../KBS/kbs.a probot.a

probot:		$(BINARIES) version.o main.o
		$(CC) -o probot version.o main.o $(PROBOT) $(BINARIES) -lm

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

probot.a:	$(PROBOT)
		ar -rc probot.a $(PROBOT)
		ranlib probot.a

install:	probot
		cp probot $(HOME)/bin
clean:
		rm -f probot *.o *.a *~ core

# DEPENDENCIES

$(PROBOT):	../interp/prolog.h probot.h read_script.h

