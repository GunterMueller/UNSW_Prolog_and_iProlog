BINDIR = 	$(HOME)/bin

PROLOG =	dump.o atom.o chartype.o eval.o evloop.o\
		files.o lex.o main.o make.o \
		mem.o out.o p_compare.o p_meta.o\
		p_db.o p_lists.o p_math.o p_system.o pred.o prove.o \
		read.o dcg.o unify.o \
		p_unix.o

CC = 		gcc
#CFLAGS	=	-g -D ALIGN -D UNIX
CFLAGS = 	-O -D UNIX
LDFLAGS =	-lm

prolog:		$(PROLOG)
		$(CC) -g -o prolog $(PROLOG) $(LDFLAGS)


install:	prolog
		cp prolog $(BINDIR)

clean:
		rm -f prolog *.o *~ core

# DEPENDENCIES

$(PROLOG):	prolog.h env.h
