CC = 		gcc

CFLAGS = 	-g -I. -Iinterp -IKBS -IML -Istats -IILP -IProbot -IGUI -Iimage\
		-I/usr/X11R6/include/X11 -I/usr/include/X11\
		-D _GNU_SOURCE

LIBS =		interp/interp.a KBS/kbs.a ML/ml.a stats/stats.a ILP/ilp.a Probot/probot.a

EXTRAS =	GUI/gui.a image/image.a

LDFLAGS =	-L/usr/X11R6/lib \
		-lm -lX11 -lXpm -lforms -lnetpbm		

all:		$(LIBS) $(EXTRAS) version.o main.o
		$(CC) -o prolog version.o main.o $(LIBS) $(EXTRAS) $(LDFLAGS)

interp/interp.a:
		(cd interp;make)

KBS/kbs.a:
		(cd KBS;make)

ML/ml.a:
		(cd ML;make)

stats/stats.a:
		(cd stats;make)

ILP/ilp.a:
		(cd ILP;make)

Probot/probot.a:
		(cd Probot;make)

GUI/gui.a:
		(cd GUI;make)

image/image.a:
		(cd image;make)

main.o:		interp/main.c
		$(CC) -c interp/main.c

install:	prolog
		cp prolog $(HOME)/bin

clean:
		rm -f prolog *.o *.a *~ core

clean_all:
		rm -f prolog *.o *.a *~ core
		(cd interp;make clean)
		(cd KBS;make clean)
		(cd ML;make clean)
		(cd stats;make clean)
		(cd ILP;make clean)
		(cd Probot;make clean)
		(cd GUI;make clean)
		(cd image;make clean)

