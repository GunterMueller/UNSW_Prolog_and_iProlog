%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%		Differentiation program.

d(U + V, X, DU + DV) :- !, d(U, X, DU), d(V, X, DV).
d(U - V, X, DU - DV) :- !, d(U, X, DU), d(V, X, DV).
d(U * V, X, U * DV + V * DU) :- !, d(U, X, DU), d(V, X, DV).
d(U / V, X, (DU * V - U * DV) / V ** 2) :- !, d(U, X, DU), d(V, X, DV).
d(U ** N, X, DU * N * U ** N1) :-  integer(N), !, N1 is N - 1, d(U, X, DU).
d(- U, X, - DU) :- !, d(U, X, DU).
d(e ** U, X, DU * e ** U) :- !, d(U, X, DU).
d(sin(U), X, DU * cos(U)) :- !, d(U, X, DU).
d(cos(U), X, - DU * sin(U)) :- !, d(U, X, DU).
d(log(U), X, DU / U) :- !, d(U, X, DU).
d(X, X, 1) :-  !.
d(C, X, 0).

d(x*x*x*x*x*x*x*x*x*x, x, X)?
d(x/x/x/x/x/x/x/x/x/x, x, X)?
d(log(log(log(log(log(log(log(log(log(log(x)))))))))), x, X)?
d((x + 1) * (x**2 + 2) * (x**3 + 3), x, X)?
