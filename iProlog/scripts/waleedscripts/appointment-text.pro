dynamic(topic/1, filter/1, backup/1)!
load('datelib.pro')!
load('convlib.pro')! 
load('netlib.pro')!
load('appointment.frame')!
script('appointment-text.script')!


datelt(date(Y1, M1, D1), date(Y2, M2, D2)) :-
	Y1 < Y2. 

datelt(date(Y1, M1, D1), date(Y2, M2, D2)) :-
	Y1 = Y2, 
	M1 < M2. 

datelt(date(Y1, M1, D1), date(Y2, M2, D2)) :-
	Y1 = Y2, 
	M1 = M2, 
	D1 < D2. 

ppevent(X) is [""] :-
	Y is s_date(X), 
	Z is today, 
	datelt(Y, Z). 

ppevent(X) is [ "On", ppdate(s_date(X)), "from", pptime(s_start(X)), "until", pptime(s_end(X)), "about", s_desc(X), " -- " ]. 

listevents is Z :-
	findall(X, f_event(X), Y), 
	map(ppevent, Y, Z). 
	
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

go :- readevents, probot(c_frontend, [init]). 
go :- probot(c_frontend, [init]). 
	
saveevent(Slots) is Result :-
	Name is newframe2(f_event, Slots), 
	date(Year, Month, Day) is s_date(Name), 
	time(SH, SM, SS)  is s_start(Name), 
	time(EH, EM, ES)  is s_end(Name), 
	EvDesc is  s_desc(Name), 
        Result is coord_query([event, add, Name, Year, Month, Day, SH, SM, SS, EH, EM, ES, EvDesc]). 

readevents :- 
	X is coord_query([event, sload]), 
	addevents(X). 

addevents([]). 

addevents([X|L]) :- 
	addevent(X), 
	addevents(L).

addevent([fillin, Date, Start, End, Desc]) :- 
	makenewframe(f_event, FrameId), 
	frame(FrameId, [f_event], [s_date(Date), s_start(Start), s_end(End), s_desc(Desc)]), !. 

addevent([FrameId, Date, Start, End, Desc]) :- 
	frame(FrameId, [f_event], [s_date(Date), s_start(Start), s_end(End), s_desc(Desc)]), !. 

	

inBox(X, Y, MinX, MaxX, MinY, MaxY) :-
	X > MinX, 
	X < MaxX, 
	Y > MinY, 
	Y < MaxY. 

