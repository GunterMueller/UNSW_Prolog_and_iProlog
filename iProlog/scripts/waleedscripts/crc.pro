dynamic(topic/1, filter/1, backup/1)!

script('crc.script')!
script('eliza.script')!
go :- probot(crc, [init]).

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
