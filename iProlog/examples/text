dynamic(sentence/1, dictionary/1, fred/0)!

politics(S, Sentence)
	where
		true
	asserting
		mkdict(S, Sentence),
		sentence(S).

(:- dictionary(W, S))
	where
		sentence(S),
		dictionary(W, S)
	asserting
		fred.

go :-
	F is refine,
	pf F,
	flush_output,
	lgg_frame(F, [finance, misc]).

mkdict(_, []) :- !.
mkdict(S, [W|Rest]) :-
	dictionary(W, S), !,
	mkdict(Rest).	
mkdict(S, [W|Rest]) :-
	assert(dictionary(W, S)),
	mkdict(Rest).

politics(1, [hospital, enquiry, defies, prime, minister]).
politics(2, [minister, gambles, on, coming, clean]).
politics(3, [minister, shifts, to, overdrive]).
finance(4, [washington, launches, trade, war, by, stealth]).
finance(5, [signs, of, life, beyond, currency, crisis]).
finance(6, [trade, drops, last, year]).
finance(7, [parliament, debates, tax, law]).
finance(8, [time, for, a, deal, on, tax, reform]).
finance(9, [tax, due, to, go, up]).
fiannce(10, [currency, crisis, hits, asia]).
finance(11, [asian, currency, problems, fade]).
misc(12, [flooded, asia, counts, the, cost]).
misc(13, [kasparov, moves, in, on, world]).
misc(14, [the, cat, sat, on, the, mat]).
misc(15, [man, bites, dog]).
