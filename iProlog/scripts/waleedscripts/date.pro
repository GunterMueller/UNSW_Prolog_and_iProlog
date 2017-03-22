dynamic(topic/1, filter/1, backup/1, drequired/1)!
script('date-new.script')?

go :- probot(frontend, [init]). 

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

% The months of the year and how many days in each month.
dom(1, _) is 31.
dom(2, Year) is 28 :- Year mod 4 =\= 0. 
dom(2, _) is 29. 
dom(3, _) is 31. 
dom(4, _) is 30. 
dom(5, _) is 31. 
dom(6, _) is 30. 
dom(7, _) is 31. 
dom(8, _) is 31. 
dom(9, _) is 30. 
dom(10, _) is 31.
dom(11, _) is 30. 
dom(12, _) is 31. 

% days of the week, converted to a number. 
dow([sunday]) is 0. 
dow([monday]) is 1. 
dow([tuesday]) is 2. 
dow([wednesday]) is 3. 
dow([thursday]) is 4. 
dow([friday]) is 5. 
dow([saturday]) is 6. 

dow([sun]) is 0. 
dow([mon]) is 1. 
dow([tue]) is 2. 
dow([wed]) is 3. 
dow([thu]) is 4. 
dow([fri]) is 5. 
dow([sat]) is 6. 

dow([[LL]]) is dow([LL]). 
dow([Cap]) is dow([lowercase(Cap)]). 

% Convert back. 

dowshort(0) is 'Sun'. 
dowshort(1) is 'Mon'. 
dowshort(2) is 'Tue'. 
dowshort(3) is 'Wed'. 
dowshort(4) is 'Thu'. 
dowshort(5) is 'Fri'. 
dowshort(6) is 'Sat'. 


dowlong(0) is 'Sunday'. 
dowlong(1) is 'Monday'. 
dowlong(2) is 'Tuesday'. 
dowlong(3) is 'Wednesday'. 
dowlong(4) is 'Thursday'. 
dowlong(5) is 'Friday'. 
dowlong(6) is 'Saturday'. 

pmonth([january]) is 1.
pmonth([february]) is 2. 
pmonth([march]) is 3. 
pmonth([april]) is 4. 
pmonth([may]) is 5. 
pmonth([june]) is 6. 
pmonth([july]) is 7. 
pmonth([august]) is 8. 
pmonth([september]) is 9.
pmonth([october]) is 10. 
pmonth([november]) is 11. 
pmonth([december]) is 12. 

pmonth([jan]) is 1.
pmonth([feb]) is 2. 
pmonth([mar]) is 3. 
pmonth([apr]) is 4. 
pmonth([may]) is 5. 
pmonth([jun]) is 6.
pmonth([jul]) is 7.
pmonth([aug]) is 8. 
pmonth([sep]) is 9. 
pmonth([oct]) is 10. 
pmonth([nov]) is 11. 
pmonth([dec]) is 12. 

pmonth([[LL]]) is pmonth([LL]). 
pmonth([Cap]) is pmonth([lowercase(Cap)]). 

monthnamelong(1) is 'January'. 
monthnamelong(2) is 'February'. 
monthnamelong(3) is 'March'. 
monthnamelong(4) is 'April'. 
monthnamelong(5) is 'May'. 
monthnamelong(6) is 'June'. 
monthnamelong(7) is 'July'. 
monthnamelong(8) is 'August'. 
monthnamelong(9) is 'September'. 
monthnamelong(10) is 'October'. 
monthnamelong(11) is 'November'. 
monthnamelong(12) is 'December'. 

dayofm([one]) is 1. 
dayofm([two]) is 2. 
dayofm([three]) is 3.
dayofm([four]) is 4. 
dayofm([five]) is 5. 
dayofm([six]) is 6. 
dayofm([seven]) is 7. 
dayofm([eight]) is 8. 
dayofm([nine]) is 9. 
dayofm([ten]) is 10. 
dayofm([eleven]) is 11. 
dayofm([twelve]) is 12. 
dayofm([thirteen]) is 13. 
dayofm([fourteen]) is 14. 
dayofm([fifteen]) is 15. 
dayofm([sixteen]) is 16. 
dayofm([seventeen]) is 17. 
dayofm([eighteen]) is 18. 
dayofm([nineteen]) is 19. 
dayofm([twenty]) is 20. 
dayofm([thirty]) is 30. 

dayofm([first]) is 1. 
dayofm([second]) is 2. 
dayofm([third]) is 3.
dayofm([fourth]) is 4. 
dayofm([fifth]) is 5. 
dayofm([sixth]) is 6. 
dayofm([seventh]) is 7. 
dayofm([eighth]) is 8. 
dayofm([ninth]) is 9. 
dayofm([tenth]) is 10. 
dayofm([eleventh]) is 11. 
dayofm([twelfth]) is 12. 
dayofm([thirteenth]) is 13. 
dayofm([fourteenth]) is 14. 
dayofm([fifteenth]) is 15. 
dayofm([sixteenth]) is 16. 
dayofm([seventeenth]) is 17. 
dayofm([eighteenth]) is 18. 
dayofm([nineteenth]) is 19. 
dayofm([twentieth]) is 20. 
dayofm([thirtieth]) is 30. 


%rule for twenties. First six characters must be "twenty", and everything from the 
% eighth character on is the day. This misses the seventh character, 
% which maybe a hyphen. The second variant explicitly allows for two words. 

dayofm([twenty, Ones]) is Result  :- 
	Day is dayofm([Ones]), 
	Day < 10, 
	Result is 20 + Day. 

dayofm([thirty, Ones]) is Result  :- 
	Day is dayofm([Ones]), 
	Day < 2, 
	Result is 30 + Day. 

dayofm([Day]) is Result  :- 
	sub_atom(Day, 1, 6, twenty), 
	atom_length(Day, DLen), 
	sub_atom(Day, 8, DLen, Ones), 
	Day is dayofm([Ones]), 
	Day < 10, 
	Result is 20 + Day. 

%% Very similar rule to the above. 

dayofm([Day]) is Result  :- 
	sub_atom(Day, 1, 5, thirty), 
	atom_length(Day, DLen), 
	sub_atom(Day, 7, DLen, Ones), 
	Day is dayofm([Ones]), 
	Day < 2, 
	Result is 30 + Day. 

dayofm([Cap]) is dayofm([lowercase(flatten(Cap))]). 

parsecard([X]) is X :- number(X). 

parsecard([one]) is 1. 
parsecard([two]) is 2. 
parsecard([three]) is 3.
parsecard([four]) is 4. 
parsecard([five]) is 5. 
parsecard([six]) is 6. 
parsecard([seven]) is 7. 
parsecard([eight]) is 8. 
parsecard([nine]) is 9. 
parsecard([ten]) is 10. 
parsecard([eleven]) is 11. 
parsecard([twelve]) is 12. 
parsecard([thirteen]) is 13. 
parsecard([fourteen]) is 14. 
parsecard([fifteen]) is 15. 
parsecard([sixteen]) is 16. 
parsecard([seventeen]) is 17. 
parsecard([eighteen]) is 18. 
parsecard([nineteen]) is 19. 
parsecard([twenty]) is 20. 
parsecard([thirty]) is 30. 
parsecard([forty]) is 40. 
parsecard([fifty]) is 50. 
parsecard([sixty]) is 60. 
parsecard([seventy]) is 70. 
parsecard([eighty]) is 80. 
parsecard([ninety]) is 90. 
parsecard([quarter]) is 15. 
parsecard([half]) is 30. 

parsecard([Tens, Ones]) is Result  :- 
	SumTens is parsecard([Tens]),
 	SumOnes is parsecard([Ones]),
	Result is SumTens + SumOnes. 



%simple definition of tomorrow. 
tomorrow is dateadd(1, today). 

%thing that provides default date. If any field has a "-1" in it, it is given 
% the default value. 

negrep(-1, Default) is Default. 
negrep(Orig, _) is Orig. 

defaultdate(date(Year, Month, Day)) is date(FixedYear, FixedMonth, FixedDay) :-
	date(ThisYear, ThisMonth, ThisDay) is today, 
	FixedYear is negrep(Year, ThisYear), 
	FixedMonth is negrep(Month, ThisMonth), 
	FixedDay is negrep(Day, ThisDay). 


% Calculate which day of the week a given day is. 
% From the Calendar FAQ - http://www.pauahtun.org/CalendarFAQ/cal/node3.html
calcday(date(Year, Month, Day)) is D :-
	A is (14 - Month) // 12, 
	Y is Year - A, 
	M is Month + 12*A - 2,
	D is (Day + Y + Y//4 - Y//100 + Y // 400 + 31*M//12) mod 7. 

%If I say for example, next Tuesday and it's Tuesday, I mean a week from now. 

calcnextweekday(Weekday) is Date :-
	ThisWeekday is calcday(today),
	GivenDay is dow(Weekday), 
	GivenDay =:= ThisWeekday, 
	Date is dateadd(7, today). 

% If I say next Wednesday and it's Tuesday, I mean tomorrow. 

calcnextweekday(Weekday) is Date :-
	ThisWeekday is calcday(today),
	GivenDay is dow(Weekday), 
	GivenDay > ThisWeekday, 
	Date is dateadd(GivenDay-ThisWeekday, today). 

% If I say next Monday and it's Tuesday, I mean six days from now. 
calcnextweekday(Weekday) is Date :-
	ThisWeekday is calcday(today),
	GivenDay is dow(Weekday), 
	GivenDay < ThisWeekday, 
	Date is dateadd(GivenDay-ThisWeekday + 7, today). 

%Function that adds date. Currently uses recursion. Adds one month at a time if the day is very far away. 


dateadd(Daystoadd, date(Year, Month, Day)) is date(Year, Month, Newday) :-
	Newday is Day + Daystoadd, Newday <= dom(Month, Year). 

dateadd(Daystoadd, date(Year, Month, Day)) is dateadd(Daysleft, date(Year, Month + 1, 1)) :-
	Newday is Day + Daystoadd, 
	Newday > dom(Month, Year), 
	Daysleft is Newday - dom(Month, Year)-1, 
	Month < 12. 

dateadd(Daystoadd, date(Year, Month, Day)) is dateadd(Daysleft, date(Year + 1, 1, 1)) :-
	Newday is Day + Daystoadd, 
	Newday > dom(Month, Year), 
	Daysleft is Newday - dom(12, Year)-1, 
	Month =:= 12. 

clearhour(X) :- 
	X <= 5. 
clearhour(X) :- 
	X > 10. 

% Hours between 1 and 5 get shifted by 12 hours. 

defaulttime(Hours, Minutes) is time(FixedHours, Minutes, 0) :-
	Hours <= 5, 
	FixedHours is Hours + 12. 

defaulttime(Hours, Minutes) is time(Hours, Minutes, 0) :-
	Hours > 10. 

defaulttime(Hours, Minutes) is time(Hours, Minutes, 0). 

minssub(Minutes, Hours, am) is time(NewHours, NewMinutes, 0) :-
	
	NewHours is parsecard(Hours) - 1, 
	NewMinutes is 60 - parsecard(Minutes). 


minssub(Minutes, Hours, pm) is time(NewHours, NewMinutes, 0) :-
	NewHours is parsecard(Hours) + 11, 
	NewMinutes is 60 - parsecard(Minutes). 

minssub(Minutes, Hours, 'default') is time(NewHours, NewMinutes, 0) :-
	RealHours is parsecard(Hours), 
	time(IntHours, _, _) is defaulttime(RealHours, 0), 
	NewHours is IntHours -1, 
	NewMinutes is 60 - parsecard(Minutes). 	


	
lowercase(Cap) is Low :-
	atom_chars(Cap, Listofchars), 
	Ll is lower(Listofchars), 
	atom_chars(Low, Ll). 

lower([]) is []. 
lower([X|L]) is [lc(X)|lower(L)] . 

% Let's implement chr() in prolog! 

lc(X) is X :-
	char_code(X, Y), 
	Y < 65. 
lc(X) is X :- 
	char_code(X, Y), 
	Y > 90. 

lc(X) is Z :- 
	char_code(X, Y), 
	W is Y + 32, 
	char_code(Z, W). 

requirefacts([]). 

requirefacts(X) :- 
	drequired(Y), 
	retract(drequired(Y)), 
	assert(drequired(X)). 
	

requirefacts(X) :- 
	assert(drequired(X)). 

gotfact(Fact) :- 
	drequired(X),
	excise(Fact, X, NewX), 
	retract(drequired(X)),
	assert(drequired(NewX)). 

needfact(Fact) :- 
	drequired(List), 
	member(Fact, List). 

gotallfacts :- drequired([null]). 

getidate is X :- idate(X). 
getitime is X :- itime(X). 
getidesc is X :- idesc(X). 

ppdate(date(Year, Month, Day)) is 
             [dowlong(calcday(date(Year, Month, Day))), Day, monthnamelong(Month), Year]. 

pptime(time(Hours, Minutes, Seconds)) is [Hours,Minutes, am] :-
	Hours <= 12.  

pptime(time(Hours, Minutes, Seconds)) is [NewHours, Minutes, pm] :-
	NewHours is Hours-12.  

	
excise(X, [], []). 
excise(X, [X|L], M) :- excise(X, L, M), !. 
excise(X, [Y|L], [Y|M]) :- excise(X, L, M). 

settime(X) :- 
	assert(itime(X)),
	needfact(itime), 
	gotfact(itime), 
	pop_topic([]). 

settime(X) :- 
	assert(itime(X)), 
	pop_topic([]). 
