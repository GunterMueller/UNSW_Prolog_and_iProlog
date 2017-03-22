dynamic(def/3, when_needed/3)!

% Get the Value of a Slot in a given Frame

% Check for a value facet
dynamic(cache/3, default/3, if_needed/3, if_added/3, if_removed/3, if_replaced/3)!

%	Check for a default value
% Check for an if_needed daemon

frame_get(Frame, Slot) is Value :-
        frame(Frame, Slot, Value).
frame_get(Frame, Slot) is Value :-
        get_rule(Frame, Slot, default, Value).
frame_get(Frame, Slot) is Value :-
        get_rule(Frame, Slot, if_needed, Frame/Expr), !,
        Value is Expr,
        check_cache(Frame, Slot, Value).

% If cache yes, store the value

check_cache(Frame, Slot, Value) :-
        get_rule(Frame, Slot, cache, yes), !,
        assertz(frame(Frame, Slot, Value)).
check_cache(_, _, _).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Put Value in Slot of a given Frame.
% If the slot has an if_added demon, execute it
% after installing the value.

frame_put(Frame, Slot, Value) :-
        get_rule(Frame, Slot, if_added, (Frame, Value)/Rule), !,
        fput(Frame, Slot, Value),
        Rule.
frame_put(Frame, Slot, Value) :-
        fput(Frame, Slot, value, Value).

% Add the new assertion into the data base

fput(Frame, Slot, Value) :-
        check_range(Frame, Slot, Value), !,
        assertz(frame(Frame, Slot, value, Value)).
fput(Frame, Slot, Value) :-
        get_rule(Frame, Slot, help, (Frame, Slot, Value)/HelpRule), !,
        HelpRule,
        fail.

check_range(Frame, Slot, Value) :-
        get_rule(Frame, Slot, range, (Frame, Value)/RangeRule), !,
        RangeRule.
check_range(_, _, _).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Remove a slot from a given frame. If the slot
% has an if_removed demon, execute it before
% removing the slot.

frame_remove(Frame, Slot) :-
        get_rule(Frame, Slot, if_removed, Frame/Rule), !,
        Rule,
        fremove(Frame, Slot).
frame_remove(Frame, Slot) :-
        fremove(Frame, Slot).

% Remove slot from the data base.
% If it doesn't exist, succeed anyway.

fremove(Frame, Slot) :-
        retract(frame(Frame, Slot, Value)), !.
fremove(_, _).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Replace whatever is in Slot with Value.
% If the slot has an if_replaced demon, execute it
% after doing the replacement.

frame_replace(Frame, Slot, Value) :-
        get_rule(Frame, Slot, if_replaced, Frame/Rule), !,
        freplace(Frame, Slot, Value),
        Rule.
frame_replace(Frame, Slot, Value) :-
        freplace(Frame, Slot, Value).

% Remove the old value and put in the new one.
% Put the old value back if fput fails.

freplace(Frame, Slot, Value) :-
        fremove(Frame, Slot),
        not(fput(Frame, Slot, Value)),
        assertz(frame(Frame, Slot, Value)).
freplace(_, _, _).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Append the value to the list in the slot.
% If the slot has an if_appended demon, execute it
% after appending the value.

frame_append(Frame, Slot, Value) :-
        get_rule(Frame, Slot, if_appended, Frame/Rule), !,
        fappend(Frame, Slot, Value),
        Rule.
frame_append(Frame, Slot, Value) :-
        fappend(Frame, Slot, Value).

% Remove the old value and put in the new one.

fappend(Frame, Slot, Value) :-
        frame(Frame, Slot, Old), !,
        ffappend(Frame, Slot, Value, Old).
fappend(Frame, Slot, Value) :-
        fput(Frame, Slot, [Value]).

ffappend(_, _, Value, Old) :-
    member(Value, Old), !.
ffappend(Frame, Slot, Value, Old) :-
    fremove(Frame, Slot),
    fput(Frame, Slot, [Value|Old]).

member(X, [X|_]) :- !.
member(X, [_|Y]) :- member(X, Y).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 % utility predicate to search hierarcy for a rule.

get_rule(Frame, Slot, Type, Rule) :-
        Type(Frame, Slot, Rule).
get_rule(Frame, Slot, Type, Rule) :-
        frame(Frame, ako, Parent),
        get_rule(Parent, Slot, Type, Rule).


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% Climb hierarchy doing all if_new daemons for all
% slots.

frame_new(Frame, Parent) :-
         assertz(frame(Frame, ako, Parent)),
         fnew(Frame).

fnew(Frame) :-
        frame(Frame, ako, Parent), !,
        fnew(Parent),
        ffnew(Frame).
fnew(Frame) :-
        ffnew(Frame).

ffnew(Frame) :-
        if_new(Frame, Slot, (Frame, Slot)/Rule),
        Rule,
        fail.
ffnew(_, _).
