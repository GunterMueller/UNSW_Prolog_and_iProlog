%%%%%%%%%%%%%%%%%%%%%% Generic Frames %%%%%%%%%%%%%%%%%%%%%%

if_needed(cylinder, radius, X/ask_user(X, radius)).
if_removed(cylinder, radius, X/frame_remove(X, cross_section)).
range(cylinder, radius, radius_range).
help(cylinder, radius, int_help).
cache(cylinder, radius, yes).

if_needed(cylinder, height, X/ask_user(X, height)).
if_removed(cylinder, height, X/frame_remove(X, volume)).
cache(cylinder, height, yes).

if_needed(cylinder, cross_section, X/(pi * radius(X)^2)).
if_removed(cylinder, cross_section, X/frame_remove(X, volume)).
cache(cylinder, cross_section, yes).

if_needed(cylinder, volume, X/(height(X) * cross_section(X))).
cache(cylinder, volume, yes).

multi(cylinder, contains, yes).
if_new(cylinder, name, X/ask_user(X, contains)).

%%%%%%%%%%%%%%%%%%%%%% Instance Frames %%%%%%%%%%%%%%%%%%%%%%

frame(c, ako, cylinder).

%%%%%%%%%%%%%%%%%%%%%%%%%% Daemons %%%%%%%%%%%%%%%%%%%%%%%%%%

ask_user(Frame, Slot) is Value :-
    prin(Slot, " of ", Frame),
    read(Value).

remove_cross_section(Frame) :-
    frame_remove(Frame, cross_section).

remove_volume(Frame) :-
    frame_remove(Frame, volume).

radius_range(Frame, Value) :-
    integer(Value).

int_help(Frame, Slot, Value) :-
    print("The ", Slot, " of ", Frame, " should be an integer").

radius(X) is frame_get(X, radius).
height(X) is frame_get(X, height).
cross_section(X) is frame_get(X, cross_section).
volume(X) is frame_get(X, volume).
