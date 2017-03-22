%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Example file for running backprop algorithm on two-input xor problem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

table xor(+input1, +input2, -output)!

xor(0, 0, 0).
xor(1, 0, 1).
xor(1, 1, 0).
xor(0, 1, 1).

go is bp(xor, [2], [learning_rate = 0.25]).

test(F) :-
	xor(X, Y, Z),
	F::xor(X, Y, Z1),
	print('Expected = ', Z, '; Actual = ', Z1).
