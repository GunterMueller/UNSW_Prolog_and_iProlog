dynamic(testpred/1)! 
script('bug.script')!
goa :- probot(buga).
gob :- probot(bugb).
goc :- probot(bugc). 
god :- probot(bugd). 

% goe: try to type in word. It should substitute today's date, but doesn't.  
% compare with gof, which does work, even though they are substantively
% the same code. 
goe :- probot(buge). 
gof :- probot(bugf). 

% This is a test of frame-making. It doesn't seem to work if called from other functions. 

load('appointment.frame')!
addframe1 :-  
	frame(f_1, [f_event], [s_date: date(2002,6,31), s_start: time(6,30,0), s_end: time(7,30,0), s_desc: blah]). 

addframe2 :-  
	Date is date(2002,6, 31), 
	frame(f_2, [f_event], [s_date: Date, s_start: time(6,30,0), s_end: time(7,30,0), s_desc: blah]). 
