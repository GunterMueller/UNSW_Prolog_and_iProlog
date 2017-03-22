KERNEL =	atom.o chartype.o dcg.o eval.o evloop.o files.o \
		lex.o make.o mem.o out.o p_compare.o p_db.o \
		plist.o p_lists.o p_math.o p_meta.o pred.o prove.o \
		p_sockets.o p_system.o p_unix.o read.o unify.o worlds.o

CC = 		gcc

CFLAGS = 	-g -I.

prolog:		version.o main.o interp.a
		$(CC) -o prolog version.o main.o interp.a -lm

interp.a:	$(KERNEL)
		ar -rc interp.a $(KERNEL)
		ranlib interp.a

install:	prolog
		cp prolog $(HOME)/bin/iprolog_lite

clean:
		rm -f prolog *.o *.a *~ core

# DEPENDENCIES

$(PROLOG):	prolog.h

