#!/usr/bin/perl
$ENV{"http_proxy"} = "http://web-2.cse.unsw.edu.au:3128/"; 

use Net::FTP; 
use Mail::POP3Client; 
use LWP::Simple; 
use XML::RSS; 
use SOAP::Lite; 

%source = (
	 "slashdot" => "http://slashdot.org/slashdot.rdf",
	 "abc" => "http://www.newsisfree.com/HPE/xml/feeds/80/2880.xml",
	 "smh" => "http://www.newsisfree.com/HPE/xml/feeds/48/3148.xml",
	 "cnn" => "http://www.newsisfree.com/HPE/xml/feeds/15/2315.xml"
	 ); 

$vcalfile = "/home/waleed/inca.vcal"; 
$vcalbak = "/home/waleed/inca.vcal.bak"; 

select((select(STDIN), $| = 1, $^ = 'mytop')[0]);
select((select(STDOUT), $| = 1, $^ = 'mytop')[0]);

while(<STDIN>){
 # print "Considering $_"; 
  /\[(.*)\]/g;
  $_ = $1; 
  @param = split(/\s*,\s*/); 
  if($param[0] eq "weather"){
    print handle_weather(@param); 
  }
  elsif($param[0] eq "echo"){
#    print "Matched echo.\n"; 
    print handle_echo(@param); 
  }
  elsif($param[0] eq "email"){
    print handle_email(@param); 
  }
  elsif($param[0] eq "news"){
    print handle_news(@param); 
  }
  elsif($param[0] eq "exchange"){
    print handle_exchange(@param); 
  }
  elsif($param[0] eq "translate"){
    print handle_translate(@param); 
  }
  elsif($param[0] eq "event"){
    print handle_event(@param); 
  }
  else {
    print "\"I really don't know what you are talking about.\".\n"; 
  }
}

sub handle_weather {
  open(WEATHER, "wget -q -O - ftp://ftp2.bom.gov.au/anon/gen/fwo/IDN10064.txt |") || return "Weather is not currently available.";
  while(<WEATHER>){
    next unless /^For.*/;
    $report ="\""; 
    while(<WEATHER>){
      last if /Precis\s+/; 
      chomp($_); 
      $report .= " ".$_; 
    }
    chomp($report); 
    last; 
  }
  while(<WEATHER>){
    next unless /^Sydney */;
    /(\d+)\s+$/; 
    $temp = $1; 
    if($temp ne ""){
      $report .= " Expected maximum: $temp degrees.\".\n";
    }
    last; 
  }
  return $report; 
}

sub handle_echo {
  my $cmdstring = shift(@_); 
  $cmdstring =~ s/^echo //; 
  return $cmdstring; 
}

sub handle_email {
  my @arg = @_;
  if($arg[1] eq "list"){
    $pop = new Mail::POP3Client("inca", "CRCincaSIT", "pop3.cse.unsw.edu.au"); 
    $rv = "\"You have ".$pop->Count." messages. "; 
    for($i=1; $i <= $pop->Count; $i++){
      foreach ($pop->Head($i)) {
	if(/^From: (.*)$/){
	  $from = $1; 
	}
	if(/^Subject: (.*)$/){
	  $subject = $1; 
	}
      }
      $from =~ s/<[^>]*>//;
      $from =~ s/\"//g; 
      $rv .= "From $from about $subject; ";
    }
    $rv .= "\".\n"; 
    return $rv; 

  }
  if($arg[1] eq "readmsg"){
    $num = $arg[2]; 
    $pop = new Mail::POP3Client("inca", "CRCincaSIT", "pop3.cse.unsw.edu.au"); 
    $message = $pop->Body($num); 
    $message =~ s/\"//g; 
    $message =~ s/\n/ /g;
    $message =~ s/\r//g;
    $rv = "\" $message\".\n\n"; 
    return $rv; 
  }
  
}

sub handle_news {
   my @arg = @_; 
   #print "a0 = $arg[0] a1 = $arg[1]\n"; 
   #print "site = >$site<\n";
   $url = $source{$arg[1]};
   #print "url = $url\n"; 
   open(NEWS, "lwp-request -p http://web-2.cse.unsw.edu.au:3128/ $url |") || return "\"$site News is not currently available.\".\n";
  $content = join("", <NEWS>); 
   close(NEWS);
   #$content = get($url); 
   if($content eq ""){
     return "\"$site news is not currently available.\".\n";
   }
   $rss = new XML::RSS;
   $rss->parse($content); 
   $rv = ""; 
   foreach $item (@{$rss->{'items'}}) {
      $rv .= $item->{'title'}."; ";
    }
   $rv =~ s/.+\"//g; 
   $rv = "\"Headlines are: ".$rv."\".\n";
   return $rv; 
}

sub handle_exchange {
  my @arg = @_;
  shift(@arg); 
  $country = join(" ", @arg); 
  if($country eq "united states"){
    $currency = "US dollars";
  }
  elsif($country eq "france"){
    $currency = "French francs";
  }
  elsif($country eq "euro"){
    $currency = "Euros"; 
  }
  elsif($country eq "japan"){
    $currency = "yen";
  }
  else {
    return "\"I don't know that currency.\".\n"; 
  }
  $response =  SOAP::Lite 
    -> uri('urn:xmethods-CurrencyExchange')
      -> proxy('http://services.xmethods.net:80/soap', proxy => ['http' => 'http://web-2.cse.unsw.edu.au:3128'])
	-> getRate("australia", $country); 
  return "\"It wasn't available. I'm sorry.\".\n"  if $response->faultcode; 
  return "\"One Australian dollar is  ".$response->result." $currency\".\n"; 
}

sub handle_translate {
  my @arg = @_;
  shift(@arg); 
  $language = shift(@arg);
  $language =~ tr/A-Z/a-z/; 
  if($language eq "french"){
    $tm = "en_fr"; 
  }
  elsif($language eq "german"){
    $tm = "en_de"; 
  }
  elsif($language eq "spanish"){
    $tm = "en_es"; 
  }
  else {
    return "\"Sorry, I can't translate into $language.\".\n";
  }
  $phrase = join(" ", @arg); 
  $response =  SOAP::Lite
    -> uri("urn:xmethodsBabelFish")
      -> proxy('http://services.xmethods.net:80/perl/soaplite.cgi', proxy => ['http' => 'http://web-2.cse.unsw.edu.au:3128'])
	-> BabelFish($tm, $phrase); 

  return "\"It wasn't available. I'm sorry.\".\n"  if $response->faultcode; 
  return "\"I think it's roughly: ".$response->result."\".\n"; 

}

sub handle_event {
  my @args = @_; 
  shift(@_); 
  $command = shift(@_); 
  if($command eq "add"){
    $eid = shift(@_); 
    ($year, $month, $day) = splice(@_, 0, 3); 
    ($sh, $sm, $sd) =splice(@_, 0, 3); 
    ($eh, $em, $ed) =splice(@_, 0, 3); 
    $desc = join(" ", @_); 
    $desc =~ s/\[//g; 
    $desc =~ s/\]//g; 
    rename($vcalfile, $vcalbak); 
    open(VCALIN, $vcalbak) || return "\"Could not open calendar for reading.\".\n"; 
    open(VCALOUT, "> $vcalfile") || return "\"Could not open calendar for writing.\".\n"; 
    while(<VCALIN>){
      if(/END\:VCALENDAR/){
	last; 
      }
      else {
	print VCALOUT; 
      }
    }
    print VCALOUT "BEGIN:VEVENT\n"; 
    print VCALOUT "UID:$eid\n"; 
    print VCALOUT "DTSTART:", tovcal($year, $month, $day, $sh, $sm, $ss), "\n"; 
    print VCALOUT "DTEND:", tovcal($year, $month, $day, $eh, $em, $es), "\n"; 
    print VCALOUT "STATUS:NEEDS ACTION\n"; 
    print VCALOUT "SUMMARY:$desc\n"; 
    print VCALOUT "END:VEVENT\n\n"; 
    print VCALOUT "END:VCALENDAR\n\n"; 
    close(VCALOUT); 
    close(VCALIN); 
    return "\"Recorded.\".\n"; 
  }
  elsif($command eq "sload"){
    $rv = "["; 
    #print "Got here\n"; 
    open(VCALOUT, "$vcalfile") || return "[].";
    while(<VCALOUT>){
      %appt = (); 
      next unless /BEGIN:VEVENT/;
      #print "Got to core.\n"; 
      while(<VCALOUT>){
	#print "Reading an event.\n";
	last if /END:VEVENT\s+/; 
	($key, $value) = split(/:/);
	$value =~ s/\n//; 
	$value =~ s/\r//; 
	$appt{$key} = $value;
      }
      if($appt{"UID"} =~ /astroboy/){
	$appt{"UID"} = "fillin"; 
      }
      $date = getvdate($appt{"DTSTART"}); 
      $start = getvtime($appt{"DTSTART"}); 
      $end = getvtime($appt{"DTEND"});
      $rv .= "frame(".$appt{"UID"}.", [f_event], [s_date($date), s_start($start), s_end($end), s_desc(`\"".$appt{"SUMMARY"}."\")]), "; 
    }
    $rv =~ s/, $//; 
    $rv .= "].\n"; 
    return $rv; 
  }
}

sub getvdate {
  $string = shift(@_); 
  $string =~ /(\d{4})(\d{2})(\d{2})T(\d{2})(\d{2})(\d{2})/; 
  $y = $1 + 0; 
  $m = $2 + 0; 
  $d = $3 + 0; 
  return "date($y, $m, $d)"; 
}

sub getvtime {
  $string = shift(@_); 
  $string =~ /(\d{4})(\d{2})(\d{2})T(\d{2})(\d{2})(\d{2})/; 
  $h = $4 + 0; 
  $m = $5 + 0; 
  $s = $6 + 0;
  return "time($4, $5, $6)"; 
}

sub tovcal {
  my ($year, $month, $day, $hours, $minutes, $seconds) = @_; 
  $retval = sprintf("%04d%02d%02dT%02d%02d%02d", $year, $month, $day, $hours, $minutes, $seconds);
  #print "\"returning $retval \".\n"; 
  #print "Retval is: $retval"; 
  return $retval;
}
