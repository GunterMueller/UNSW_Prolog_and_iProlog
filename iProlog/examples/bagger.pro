%% Test program for skeletonisation and tracing

go :-
	read_pgm('test.pgm'),
	display_pgm,
	threshold(140),
	display_pbm,
	skeletonise,
	display_pbm,
	trace_skeleton.
