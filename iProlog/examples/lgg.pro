lgg(
	f(g(a, b), [1, 2, [3, 4], 5], 1 + 2 * 3),
	f(g(a, h(a, b)), [1, 2, [3, 4, 5]], 1 + 6),
	X,
	Y
)?

lgg(
	(q(f(a)) :- p(a, b), r(b, c), r(b, e)),
	(q(f(x)) :- p(x, y), r(y, z), r(w, z)),
	X,
	Y
)?