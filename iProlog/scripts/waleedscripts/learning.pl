rord(high,home,noisy,display).
rord(high,office,quiet,display). 
rord(high,car,quiet,read).
rord(high,party,noisy,display). 
rord(low,office,quiet,read).
rord(low,car,noisy,read). 
rord(low,home,quiet,read). 
table rord(
        resolution(high,low),
        environment(office,car,party,home),
        noiselevel(quiet,noisy),
        outcome(display,read)
).
