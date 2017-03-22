quick_sort(L0, L) :- qsort(L0, L, []).

qsort([], R, R).
qsort([X|L], R, R0) :-
	partition(L, X, L0, L1), 
	qsort(L1, R1, R0), 
	qsort(L0, R, [X|R1]).

partition([], _, [], []).
partition([X|L], Y, [X|L0], L1) :-
	X < Y, !,
	partition(L, Y, L0, L1).
partition([X|L], Y, L0, [X|L1]) :-
	partition(L, Y, L0, L1).

data([1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
	25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,
	47,48,49,50]).

bench(Count) :-
	T0 is cputime,
	dobench(Count),
	Time is cputime - T0,
	print("Time = ", Time).

dobench(Count) :-
	data(List),
	repeatN(Count),
	quick_sort(List,_),
	fail.
dobench(_).

repeatN(N).
repeatN(N) :- N > 1, N1 is N-1, repeatN(N1).

%bench(100)!
