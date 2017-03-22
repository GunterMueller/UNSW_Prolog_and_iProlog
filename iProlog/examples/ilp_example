%%Prolog

% modeh(*, p(+integer)).
% modeb(*, q(+integer)).
% modeb(*, r(+integer, -integer)).
% modeb(*, m(+integer, -integer)).

modeh(1, mem(+char,+list)).
modeb(1, mem(+char,+list)).
%modeb(1, (+list) = (-list)).
%modeb(1, (+char) = (#char)).

char(a).
char(b).
char(c).
char(d).
char(e).
char(f).
char(g).
char(h).
char(i).
char(j).
char(k).
char(l).
char(m).
char(n).
char(o).
char(p).
char(q).
char(r).

any(_).

list([]).
list([H|T]):-
	any(H),
	list(T).

types([char, list, any]).

mem(a,[b,a,a,c,d]).
mem(a,[a,a,c,d]).
mem(a,[a,c,d]).
mem(d, [d]).

go :- saturate(mem(a,[b,a,a,c,d])).

p(1).
p(2).
p(3).

q(X) :- X < 4.
r(X, 2).
r(X, 3).
m(3, 4).

false:- member(X,[5,6,7]), p(X).

% false:- p(6).
% false:- p(7).
% false:- p(5).
