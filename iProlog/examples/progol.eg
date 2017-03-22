modeh(*, reverse(+list, -list)).
modeb(*, +any = #any).
modeb(*, conc(+list, [+int], -list)).
modeb(*, +list = [-int | -list]).
modeb(*, reverse(+list, -list)).

types([int, list, any]).

int(1).
int(2).
int(3).

any(_).

list([]).
list([H|T]):- list(T).

reverse([], []).

conc([], X, X).
conc([H|T], L1, [H|L2]) :- conc(T, L1, L2).

go :- saturate(reverse([1], [1])).
