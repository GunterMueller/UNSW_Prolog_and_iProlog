dynamic(topic/1, filter/1, backup/1, drequired/1, tmptime/1)!

script('unsw.script')? 

putprop(global, locn, parking)!

new_topic(Topic, Filter, Backup) :-
	topic(Topic), !.

new_topic(Topic, Filter, Backup) :-
	asserta(topic(Topic)),
	asserta(filter(Filter)),
	asserta(backup(Backup)).

pop_topic(Sent) :-
	retract(topic(_)), !,
	retract(filter(_)), !,
	retract(backup(_)), !,
	topic(OldTopic), !,
	goto(OldTopic, Sent).


get_location is X :-
	X is getprop(global, locn). 

set_location(X) :- 
	remprop(global, locn), 
	putprop(global, locn, X). 

go :- probot(c_frontend, [init]). 