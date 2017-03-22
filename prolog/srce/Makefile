# Makefile for UNSW Prolog V4.2

FILES	= g.h in.h pred.h\
	  lex.c main.c mem.c out.c parse.c pred.c prove.c make.c\
	  p_IO.c p_basic.c p_behave.c p_clause.c p_meta.c\
	  p_unix.c p_streams.c monitor.c profile.c spaces.c

OBJECTS	= main.o mem.o out.o pred.o prove.o make.o monitor.o profile.o spaces.o
INPUTS	= lex.o parse.o
PREDEFS	= p_basic.o p_behave.o p_meta.o p_clause.o
IOPRED	= p_IO.o p_streams.o p_unix.o

.c.o:	; cc -O -c $*.c

prolog:	$(OBJECTS) $(INPUTS) $(PREDEFS) $(IOPRED)
	cc -O -o prolog $(OBJECTS) $(INPUTS) $(PREDEFS) $(IOPRED) -lm

install:	prolog
		strip prolog
		cp prolog /tmp_amd/dexter/home/cs/ai/dexter/1/claude/bin/prolog

$(OBJECTS):	g.h

$(INPUTS):	g.h in.h

$(PREDEFS):	g.h pred.h

$(IOPRED):	g.h pred.h in.h

clean:
	-rm -f $(OBJECTS) $(INPUTS) $(PREDEFS) $(IOPRED)
