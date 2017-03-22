#!/usr/bin/perl -w

use Date::Manip;

while(<STDIN>){
  $date = &ParseDate($_); 
  print &UnixDate($date, "date(%Y, %m, %d), time(%H, %M, %S)\n"); 
}
