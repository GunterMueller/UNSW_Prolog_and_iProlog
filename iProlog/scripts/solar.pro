dynamic(topic/1, filter/1, backup/1)!

script('../scripts/solar.script')!

go :- probot(solar_system, [init]).

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
	change_topic(OldTopic, Sent).
