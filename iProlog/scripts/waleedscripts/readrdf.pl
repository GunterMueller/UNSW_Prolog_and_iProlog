#!/usr/bin/perl -w

use XML::RSS; 

$rss = new XML::RSS;
$rss->parsefile("slashdot.rdf"); 

foreach $item (@{$rss->{'items'}}) {
  print "title: $item->{'title'}\n";
  print "link: $item->{'link'}\n\n";
}

