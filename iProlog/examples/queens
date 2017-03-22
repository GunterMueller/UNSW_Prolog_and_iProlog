%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%	N-queens program.
%	Written by Rowland Sammut.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

choose(X, [X | Y], Y).
choose(X, [H | T1], [H | T2]) :-  choose(X, T1, T2).

safe(U1, D1, [square(_, U, D) | Rest]) :-
	U1 =\= U,
	D1 =\= D,
	safe(U1, D1, Rest).
safe(_, _, []).

solve(Input, Output, [Row | R], Columns) :-
	choose(Col, Columns, C),
	Up is   Row - Col,
	Down is Row + Col,
	safe(Up, Down, Input),
	solve([square(Col, Up, Down) | Input], Output, R, C).
solve(L, L, [], []).

inlist(1, L, [1 | L]).
inlist(X, L1, L2) :-
	X > 1,
	Y is X - 1,
	inlist(Y, [X | L1], L2).
	
queens(N, Solution) :-
	inlist(N, [], L),
	solve([], Solution, L, L).

write_dots(X) :-
        X > 0,
        prin(' .'),
        Y is X - 1,
        write_dots(Y).
write_dots(0).
 
print_board(N, [square(X, _, _) | L]) :-
        nl,
        Y is X - 1,
        write_dots(Y),
        prin(' Q'),
        W is N - X,
        write_dots(W),
        print_board(N, L).
print_board(N, []) :- nl.

nq(N) :- queens(N, S), print_board(N, S), fail.
nq(_).

nq_t(N) :- queens(N, S), fail.
nq_t(_).

time :-
        T0 is cputime,
        nq_t(8),
        T1 is cputime - T0,
        print('Time = ', T1).

%nq(8)?
