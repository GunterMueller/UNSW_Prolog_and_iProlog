makenewframe(X, Z) :-
	repeat, 
	Y is floor(random(0,1000000)),
	myconcat(X, Y, Z),
	not(X(Z)), !. 

myconcat(X, Y, Z) :-
	atom_chars(Z, [X, Y]). 
	

newframe(GenFrame, Slots) :- 
	makenewframe(GenFrame, Z), 
	FrameName is Z, 
	frame(FrameName, [GenFrame], Slots). 

newframe2(GenFrame, Slots) is FrameName :- 
	makenewframe(GenFrame, Z), 
	FrameName is Z, 
	frame(FrameName, [GenFrame], Slots). 
	
map(_, [], []). 
map(Function, [X|L], [Y|Rest]) :-
	Y is Function(X), 
	map(Function, L, Rest). 
