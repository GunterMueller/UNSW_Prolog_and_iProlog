acceptable(Applicant) :-
	nominated(Applicant),
	eligible(Applicant).

nominated(Applicant) :-
	nominated_by( Applicant, Member1),
	nominated_by(Applicant, Member2),
	Member1 \= Member2,
	current_year(ThisYear),
	joined(Member1, Year1),
	ThisYear >= Year1 + 2,
	joined(Member2, Year2),
	ThisYear >= Year2 + 2.

eligible(Applicant) :-
	graduated(Applicant, University),
	university(University),
	experience(Applicant, Experience),
	Experience >= 2,
	fee_payed(Applicant).

experience(fred, 3).
fee_payed(fred).
graduated(fred, unsw).
university(unsw).
nominated_by(fred, jim).
nominated_by(fred, mary).
joined(jim, 1993).
joined(mary, 1994).

current_year(1997).
