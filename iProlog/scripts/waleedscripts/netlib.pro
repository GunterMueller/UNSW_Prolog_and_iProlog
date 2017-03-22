coord_connect :-
        open_tcpip_socket(localhost, 9006, X), 
        %% open("bout.txt", "w", Out),
        %% open("bin.txt", "r", In),
        assert(fileio(X, X)),
        %% assert(coordsock(X)), 
	coord_write(coord). 

coord_close :-
	coordsock(X), 
	close(X). 

coord_ask(X) is Y :-
	coord_write(X), 
	Y is coord_read. 

coord_write(X) :- 
	current_output(OldOut), 	
        %coordsock(CoordSock), 
        fileio(In, Out), 
	set_output(Out), 
	print(X), 
        flush_output, 
	set_output(OldOut). 

coord_read is Y :-
	current_input(OldIn), 	
	coordsock(CoordSock), 
	set_input(CoordSock), 
	read(Y), 
	set_input(OldIn). 

coord_query(X) is ReturnValue :-
	open_tcpip_socket(localhost, 9006, CoordSock), 
	current_output(OldOut), 	
	current_input(OldIn), 	
	set_output(CoordSock), 
	set_input(CoordSock),  
 	print(coord),
	print(X), 
	flush_output,
	read(ReturnValue),
	set_output(OldOut), 
	set_input(OldIn). 

try_read(X) :-
	read(X), !.
try_read(bugger).


