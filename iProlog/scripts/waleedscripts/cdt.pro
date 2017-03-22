% -*- prolog -*- 
dynamic(topic/1, filter/1, backup/1)!
load('datelib.pro')!
load('convlib.pro')! 
load('netlib.pro')!
script('cdt.script')!

putprop(context,docs, [])!

getnextstate is NextState :- nextstate(NextState). 

fs_context(X) is ReturnValue:-
	OldContext is getprop(context,docs),
	append(X, OldContext, Request),
	fs(ReturnValue,CurrentContext) is coord_query(Request), 
	remprop(context, docs), 
	putprop(context, docs, CurrentContext).

focus(X) :-
	CurrentDocs is getprop(context,docs),
	DocIndex is X-1,
	NewDoc is nth(DocIndex,CurrentDocs), 
	remprop(context,docs),
	putprop(context,docs, [NewDoc]).

go :- probot(c_frontend, [init]). 

	
nth(0, [Head|Rest]) is Head.

nth(Index, []) is false. 
nth(Index, [Head|Rest]) is nth(Index-1, Rest). 

