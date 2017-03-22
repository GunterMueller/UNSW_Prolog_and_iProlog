
aff(0, [anyof([[yes], [ok], [fine], [no, problem], [sure], [whatever, you, say]])], []).

backup(0, ["*"], nextof([["Even", though, "Babbage\'s", work, was, never, finished, ",", there, were, some, important, "side-effects", of, his, research, ".", "Machining", and, manufacturing, technology, improved, as, a, result, of, attempts, to, build, the, calculating, machines, "."], [context(ada, [init])], [context(filter, [boring])]])).

babbage_no(0, [init], nextof([["The", only, technology, "Babbage", had, available, to, him, consisted, of, wheels, ",", gears, and, steam, power, ".", "His", calculating, engines, were, more, complicated, than, anything, anyone, had, yet, built, ".", context(babbage_spinoff)]])).

neg(0, [anyof([[no], [never], [not, on, your, life], ["*", off]])], []).

babbage(0, [init], nextof([["In", the, mid, "1800\'s", ",", the, "English", mathematician, ",", "Charles", "Babbage", ",", designed, a, general, purpose, mechanical, calculating, machine, that, anticipated, the, principles, and, structure, of, the, modern, computer, ".", "Do", you, know, why, he, was, never, able, to, finish, building, his, "Analytical", "Engine", and, why, we, "didn\'t", have, mechanical, computers, "100", years, earlier, "?"]])).
babbage(0, [\ aff], nextof([["OK", ",", tell, me, ".", "Why", "?", context(babbage_yes)]])).
babbage(0, [anyof([["*", why, "*"], [\ neg]])], nextof([[context(babbage_no, [init])]])).

babbage_yes(0, ["*", anyof([[gears], [complicated], [mechanical]]), "*"], nextof([["That\'s", right, ".", "The", only, technology, "Babbage", had, available, to, him, consisted, of, wheels, ",", gears, and, steam, power, ".", "His", calculating, engines, were, more, complicated, than, anything, anyone, had, yet, built, ".", context(babbage_spinoff)]])).
babbage_yes(0, ["*"], nextof([[context(babbage_no, [init])]])).

ada(0, [init], nextof([["Apart", from, "Charles", "Babbage", ",", someone, else, connected, with, his, project, interests, me, ".", "Have", you, heard, of, "Lady", "Lovelace", "?"]])).
ada(0, [\ aff], nextof([["So", you, know, that, she, was, the, "world\'s", first, programmer, "."]])).
ada(0, [anyof([["*", anyof([["Ada"], ["Lovelace"]]), "*"], [\ neg]])], nextof([["Lady", "Lovelace", was, "Charles", "Babbage\'s", assistant, and, the, "world\'s", first, computer, programmer, ".", "Her", first, name, was, "\"Ada\"", and, now, there, is, a, programming, language, called, "Ada", ",", in, here, honor, "."]])).

filter(0, ["*", anyof([[boring], [bored], [something, else], [not, interested]]), "*"], nextof([["We\'re", very, proud, to, have, a, component, of, "Babbage\'s", "Difference", "Engine", "Number", "1", in, this, museum, ".", "You", can, see, it, ",", and, an, amazing, digital, reconstruction, of, the, whole, machine, ",", right, here, in, this, exhibition, "."]])).
filter(0, ["*", anyof([["Ada"], ["Lovelace"]]), "*"], nextof([[context(ada, ^ 0)]])).
