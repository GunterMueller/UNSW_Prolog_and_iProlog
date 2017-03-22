STATS =		array.o discrim.o line.o mreg.o

CC = 		gcc

CFLAGS = 	-g -I. -I../interp -I../ML

BINARIES =	../interp/interp.a ../ML/ml.a stats.a

LDFLAGS =	-lm

stats:		$(BINARIES) version.o main.o
		$(CC) -o prolog_stats version.o main.o $(BINARIES) $(LDFLAGS)

stats.a:	$(STATS)
		ar -rc stats.a $(STATS)
		ranlib stats.a

main.o:		../interp/main.c
		$(CC) -c ../interp/main.c

install:	stats
		cp prolog_stats $(HOME)/bin

clean:
		rm -f prolog_stats *.o *.a *~ core

# DEPENDENCIES

$(STATS):	../interp/prolog.h

