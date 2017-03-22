#!/usr/bin/perl
$ENV{"http_proxy"} = "http://www-proxy.cse.unsw.edu.au:3128/"; 

use Net::FTP; 
use Mail::POP3Client; 
use LWP::Simple;
use LWP::UserAgent;  
use XML::RSS; 
#use SOAP::Lite ( +trace => all, maptype => {}  ) ;
use SOAP::Lite; 
use HTTP::Cookies; 
use HTTP::Request; 
use HTTP::Response; 
use XML::Simple;
use Data::Dumper; 
use URI::Escape; 
use MIME::Lite; 
use HTML::Entities; 

@statustypes = ("checked in", "checked out", "finalized", "pending approval"); 

%source = (
	 "slashdot" => "http://slashdot.org/slashdot.rdf",
	 "abc" => "http://www.newsisfree.com/HPE/xml/feeds/80/2880.xml",
	 "smh" => "http://www.newsisfree.com/HPE/xml/feeds/48/3148.xml",
	 "cnn" => "http://www.newsisfree.com/HPE/xml/feeds/15/2315.xml"
	 ); 

%urns = (
	 "os"  => "http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#",
	 "dc"  => "http://dublincore.org/",
	 "cdt" => "http://www.creative.com.au/schemas/FileSphere#",
	 "mp3" => "http://www.Neurocom.com.au/EDMS/Schemas/MP3#",
	 "msword" => "http://www.Neurocom.com.au/EDMS/Schemas/Office/Doc#",
	 "pdf" => "http://www.Neurocom.com.au/EDMS/Schemas/PDF#",
	 "msexcel" => "http://www.Neurocom.com.au/EDMS/Schemas/Office/MsExcel#",
	 "email" => "http://www.Neurocom.com.au/EDMS/Schemas/EMail#",
);


%addresses = (
	   "claude" => "claude\@cse.unsw.edu.au",
	   "waleed" => "waleed\@cse.unsw.edu.au",
	   "roger" => "rkermode\@arc.corp.mot.com",
	   "peter" => "pbeadle\@arc.corp.mot.com",
	   "gautam" => "gautam.tendulkar\@smartinternet.com.au",
	   "karim" => "Karim.Barbara\@team.telstra.com",
	   "tim" => "Tim.Hibberd\@team.telstra.com",
	   "satya" => "satya.anupindi\@team.telstra.com",
	   "john" => "jjzic\@arc.corp.mot.com",
	   "bahram" => "bahram\@creative.com.au",
	      "mohsen" => "mohsen\@creative.com.au"
	     
	   );


$vcalfile = "/home/waleed/inca.vcal"; 
$vcalbak = "/home/waleed/inca.vcal.bak"; 

@card = ("", "first", "second", "third", "fourth", "fifth", "sixth", "seventh",
	 "eighth", "ninth", "tenth", "eleventh", "twelfth", "thirteenth", "fourteenth", "fifteenth", "sixteenth", "seventeenth", "eighteenth", "nineteenth");

$fs_server="192.168.250.128";


select((select(STDIN), $| = 1, $^ = 'mytop')[0]);
select((select(STDOUT), $| = 1, $^ = 'mytop')[0]);

#Code to initialise cookies and the like.
#for xml methods. 

$ua = LWP::UserAgent->new(); 
$cookiejar = HTTP::Cookies->new(); 

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
  elsif($param[0] eq "fs"){
    print handle_filesphere(@param); 
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
      $report .= " Expected maximum: $temp degrees.";
    }
    last; 
  }
  $report .= "\".\n";
  return $report; 
}

sub handle_echo {
  my $cmdstring = shift(@_); 
  $cmdstring =~ s/^echo //; 
  return $cmdstring; 
}

sub get_from_fs_debug {
  my $getstring = shift(@_);
  print "GET is: ", $getstring, "\n"; 
  my $uri = "http://192.168.250.128:8080/EDMSWebService.asmx/Peers_Get?peerip=sandmanxp&actorType=LocalGroup&username=administrator&password=Admin HTTP/1.1";
  my $request = HTTP::Request->new(GET=> $uri); 
  my $response = $ua->request($request); 
  $cookiejar->extract_cookies($response); 
  #print "Ok, the Response is: ", $response->content(), "\n"; 
  $uri = "http://192.168.250.128:8080/EDMSWebService.asmx/$getstring";
  print "URI is: <", $uri, ">\n"; 
  $request = HTTP::Request->new(GET=> $uri); 
  $cookiejar->add_cookie_header($request); 
  $response = $ua->request($request); 
  print "Ok, the Response is: ", $response->content(), "\n"; 
  if($response->is_error){
      print "There was an error. Error was: ", $response->error_as_HTML();
      return ""; 
    }
  else {
    my $xmlfile = $response->content;
    $xmlfile =~ s/utf-16/utf-8/; 
    print "XML File is: ", $xmlfile, "\n"; 
    my $ref = XMLin($xmlfile);
    print "Doc is: ", Dumper($ref), "\n";
    my $doc2 = $ref->{"content"}; 
    $doc2 =~ s/&/&amp;/g; 
    my $ref2 = XMLin($doc2);
    print "Doc2 is: ", Dumper($ref2), "\n"; 
    return $ref2; 
  }
}

sub get_from_fs {
  my $getstring = shift(@_);
  my $uri = "http://192.168.250.128:8080/EDMSWebService.asmx/Peers_Get?peerip=sandmanxp&actorType=LocalGroup&username=administrator&password=Admin HTTP/1.1";
  my $request = HTTP::Request->new(GET=> $uri); 
  my $response = $ua->request($request); 
  $cookiejar->extract_cookies($response); 
  $uri = "http://$fs_server:8080/EDMSWebService.asmx/$getstring";
  $request = HTTP::Request->new(GET=> $uri); 
  $cookiejar->add_cookie_header($request); 
  $response = $ua->request($request); 
  if($response->is_error){
      #print STDERR "There was an error. Error was: ", $response->error_as_HTML();
      return ""; 
    }
  else {
    my $xmlfile = $response->content;
    $xmlfile =~ s/utf-16/utf-8/; 
    #print "XML File is: ", $xmlfile, "\n"; 
    my $ref = XMLin($xmlfile);
    #print "Doc is: ", Dumper($ref), "\n";
    my $doc2 = $ref->{"content"}; 
    $doc2 =~ s/&/&amp;/g; 
    my $ref2 = XMLin($doc2); 
    #print "Doc2 is: ", Dumper($ref2), "\n"; 
    return $ref2; 
  }
}

sub get_from_fs_single {
  my $getstring = shift(@_);
  my $uri = "http://192.168.250.128:8080/EDMSWebService.asmx/Peers_Get?peerip=sandmanxp&actorType=LocalGroup&username=administrator&password=Admin HTTP/1.1";
  my $request = HTTP::Request->new(GET=> $uri); 
  my $response = $ua->request($request); 
  $cookiejar->extract_cookies($response); 
  $uri = "http://$fs_server:8080/EDMSWebService.asmx/$getstring";
  $request = HTTP::Request->new(GET=> $uri); 
  $cookiejar->add_cookie_header($request); 
  $response = $ua->request($request); 
  if($response->is_error){
      #print STDERR "There was an error. Error was: ", $response->error_as_HTML();
      return ""; 
    }
  else {
    my $xmlfile = $response->content;
    $xmlfile =~ s/utf-16/utf-8/; 
    #print "XML File is: ", $xmlfile, "\n"; 
    my $ref = XMLin($xmlfile);
    #print "Doc is: ", Dumper($ref), "\n";
    return $ref; 
  }
}

sub maintaincontext {
  
}

sub handle_filesphere {
  my @arg = @_;
  my $what, $string, $query, $result, $getstring; 
  #Handles Schema enumeration
  if($arg[1] eq "listtypes"){
    $result = get_from_fs("Schemas_Enum?"); 
    if($result ne ""){
      @schemas = (); 
      foreach $x (@{$result->{"arrayList"}->{"anyType"}}){
	push(@schemas, $x->{"content"});
	
      }
      @origcontext = @arg;
      splice(@origcontext,0,2);
      return "fs(\"I can handle the following formats: ", join(", ", @schemas), "\", [",join(",", @origcontext)."]).\n";
    }
    else {
      return "fs(\"I had a problem talking to the server.\",[]).\n"
    }
    #print "DEBUG: ", join(", ", @schemas), "\n"; 
  }

  if($arg[1] eq "resetcontext"){
    $string = "Select ?Z from * Where (?X, <OS:Name>, ?Z) AND ?Z NOTLIKE '~~~' using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";
    $query = uri_escape($string); 
    @currcontext = (); 
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    foreach $x (@{$result->{"rdf:Description"}}){
      push(@currcontext, $x->{"rdf:about"}); 
    }
  return "fs(\"The context is reset to all documents.\",[".join(",",@currcontext)."]).\n";

  }
  if($arg[1] eq "intray"){
    $result = get_from_fs("Activities_EnumReceived");
   if($result ne ""){
     @actids = ();
     @currcontext = ();
     foreach $x (@{$result->{"arrayList"}->{"anyType"}}){
       #print "In for: $x\n";
       push(@actids, $x->{"content"});
     }
     if($result ne ""){
       @names = ();
       foreach $actid (@actids){
	 $getstring = "Activities_Get?actvID=$actid"; 
	 $result = get_from_fs($getstring);
	 #print "Docid is: ".$result->{"DocID"}."\n";
	 $docid = $result->{"DocID"};
	 push(@currcontext, $docid); 
	 $getstring = "Documents_Get?docID=$docid"; 
	 $result = get_from_fs($getstring);
	 if($result ne ""){
	   my $serxml = $result->{"metadata"}->{"SerXml"};
	   $serxml =~ s/utf-16/utf-8/g; 
	   $serxml =~ s/&lt;/</g; 
	   $serxml =~ s/&gt;/>/g; 
	   #print "SerXML is: $serxml \n"; 
	   my $newref = XMLin($serxml);
	   #print "NEW REF: ", Dumper($newref);
	   push(@names, $newref->{"rdf:Description"}->{"OperatingSystem:Name"});
	 }
       }
       @names = grep(s/\.\w{1,3}$//, @names); 
       @namescard = (); 
       for($i=0; $i <= $#names; $i++){
	 push(@namescard, "The ".$card[$i+1]. " is: ".$names[$i]); 
       }
       return "fs(\"You have ".($#names+1)." documents in your In Tray. ".join(", ", @namescard), ".\", [".join(",",@currcontext)."]).\n";
     }
     
   }
    else {
      return "fs(\"Sorry. I couldn't connect to FileSphere.\",[]).\n"; 
    }
  }
  if($arg[1] eq "sendmail"){
    my $rcvr = $arg[2];
    my $email = $addresses{$rcvr};
    #print "Receiver is: $rcvr -- $email\n";

    my $docid = $arg[3];
    @origcontext = @arg;
    splice(@origcontext, 0,3);
    $files = join(",", @origcontext);
    $result = get_from_fs("Document_EnumVersions?DocID=$docid&versions=All");
    #print "Result is: ", Dumper($result);
    @versions = @{$result->{"arrayList"}->{"anyType"}};
    %latestver = %{pop(@versions)};
    $latest = $latestver{"content"};
    #print "Latest version is: $latest\n";

    $result = get_from_fs("Document_GetVersion?DocID=$docid&version=$latest");
    #print "Result is: ", Dumper($result);
    $serxml = $result->{"file"};    
    $serxml =~ s/utf-16/utf-8/g; 
    $serxml =~ s/&lt;/</g; 
    $serxml =~ s/&gt;/>/g; 
    $newres = XMLin($serxml);
   # print "NEW Result is: ", Dumper($newres);
    # Now, let's try to copy the file:
    $id = $newres->{"id"};
    $result = get_from_fs_single("File_CopyTo?FileId=$id");
    #print "Result is: ", Dumper($result);
    $filename = $result->{"content"};
    $filename=~ /([a-z0-9-]+)\\$id$/g;
    $storedir = $1;
    #print "storedir is $storedir\n";
    #Now get the actual file
    my $uri = "http://192.168.250.128:8080/Temp/$storedir/$id";
    my $request = HTTP::Request->new(GET=> $uri); 
    my $response = $ua->request($request); 
    if($response -> is_error){
      print "There was an error. Error was: ", $response->error_as_HTML();
      return "fs(\"I couldn't retrieve the file to send\",[$files]).\n"; 
    }
    my $downfile = $response->content;
    #print "Saving in /tmp/$id\n";
    open(TMPFILE, ">/tmp/$id") || print "Could not open outputfile";
    print TMPFILE $downfile;
    close(TMPFILE);
    #Now mail it
    #print("Emailing to $email ...\n");
    $msg = MIME::Lite->new(
			   From => 'inca@cse.unsw.edu.au',
			   To => "$email",
			   Subject => 'Inca File Request',
			   Type => 'TEXT',
			   Data => "Dear Sir/Madam,\n\nMr Waleed Kadous, my boss, has asked me to forward this file to you\n\nRegards,\n\n\InCA",
			   );
    $msg->attach(
		 Type => 'application/octet-stream',
		 Path => "/tmp/$id",
		 Filename => "$id"
		 );
    MIME::Lite->send('smtp', 'smtp.cse.unsw.edu.au', Timeout => 60);
    
    $msg->send; 
		 
    return ("fs(\"Ok, the file has been sent.\",[$files]).\n");
    
    
    
  }
  if($arg[1] eq "sizecurrent"){
    @origcontext = @arg;
    splice(@origcontext, 0,2);
    $files = join(",", @origcontext);
    $string = "Select ?N, ?S From $files Where (?X, <OS:Name>, ?N), (?X, <OS:Size>,?S) using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";
    
    $query = uri_escape($string); 
    #print "List? WTF? \n"; 
    #print "String is: ", $query, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    if($result ne ""){
      #print Dumper($result); 
      #$count = $result->{"rdf:Description"}->{"DocumentsCount"}; 
      @docs = ();
      @sizes = (); 
      foreach $x (@{$result->{"rdf:Description"}}){
	#print "In there with $x\n";
	push(@docs, $x->{"OperatingSystem:Name"});
	push(@sizes, $x->{"OperatingSystem:Size"});
      }
      #print "Current context is: ", join(" ", @currcontext); 
      @docs = grep(s/\.\w{1,3}$//, @docs); 
      @docsout = (); 
      for($i=0; $i <= $#docs; $i++){
	push(@docsout, $docs[$i]." is ".$sizes[$i]." bytes"); 
      }
      return 'fs("'.join(", ", @docsout).",[$files]).\n";
    }
    else {
      return "fs(\"Sorry. I couldn't connect to file sphere.\", [".$files."]).\n";
    }
  }
  if($arg[1] eq "current"){
    @origcontext = @arg;
    splice(@origcontext, 0,2);
    $files = join(",", @origcontext);
    $string = "Select ?Z From $files Where (?X, <OS:Name>, ?Z) using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";
    
    #print "STRING is: $string\n";
    $query = uri_escape($string); 
    #print "List? WTF? \n"; 
    #print "String is: ", $query, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    if($result ne ""){
      #print Dumper($result); 
      #$count = $result->{"rdf:Description"}->{"DocumentsCount"}; 
      @schemas = (); 
      foreach $x (@{$result->{"rdf:Description"}}){
	push(@schemas, $x->{"OperatingSystem:Name"});
      }
      #print "Current context is: ", join(" ", @currcontext); 
      @schemas = grep(s/\.\w{1,3}$//, @schemas); 
      @schemascard = (); 
      for($i=0; $i <= $#schemas; $i++){
	push(@schemascard, "The ".$card[$i+1]. " is: ".$schemas[$i]); 
      }
      return "fs(\"There are ".($#schemas+1)." current documents. ".join(", ", @schemascard), ".\",[$files]).\n";
    }
    else {
      return "\"Sorry. I couldn't connect to file sphere.\".";
    }
  }
  if($arg[1] eq "genquery"){
    my $doctype = $arg[2]; 
    my $fieldname = $arg[3]; 
    @origcontext = @arg;
    splice(@origcontext, 0,4);
    $files = join(",", @origcontext);
    $string = "Select Distinct ?Z from * Where (?X, <$doctype:$fieldname>, ?Z) using $doctype for <http://www.Neurocom.com.au/EDMS/Schemas/$doctype#>";
    $query = uri_escape($string); 
    #print "Query? WTF?\n "; 
    # print "String is: ", $, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    if($result ne ""){
      @schemas = (); 
      foreach $x (@{$result->{"rdf:Description"}}){
      push(@schemas, $x->{"$doctype:$fieldname"});
	
      }
      return "fs(\"The $fieldname in your $doctype files are: ". join(", ", @schemas), ".\",[$files]).\n";  
    }
    else {
      return "fs(\"Wow ... I'm not sure I totally understood you.\",[$files]).\n";
    }
  }
  if($arg[1] eq "currquery"){
    my $service = $arg[2];
    my $urn = $urns{$service}; 
    #print "Service is: $urn\n"; 
    my $fieldname = $arg[3];
    @origcontext = @arg;
    my $os = $urns{"os"}; 
    splice(@origcontext, 0,4);
    $files = join(",", @origcontext);
    $string = "Select ?Y, ?Z from $files Where (?X, <DocType:$fieldname>, ?Z), (?X,<OS:Name>,?Y) using DocType for <$urn>, OS for <$os>";
    $query = uri_escape($string); 
    #print "Query? WTF?\n ";
    #print "String is: ", $string, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    ($whatstring) = $urn =~ m:.*/([a-zA-Z0-9\._-]+)[#/]?$:;
		       #print "Whatstring is $whatstring\n";
    if($result ne ""){
      my $count =0; 
      @schemas = (); 
      foreach $x (@{$result->{"rdf:Description"}}){
	$count++;
	my $element = decode_entities($x->{"$whatstring:$fieldname"});
	$element =~ s/"//g;
	push(@schemas, "The ".$card[$count]." has $fieldname ".$element);

      
      }
      
      return "fs(\"". join(", ", @schemas), ".\",[$files]).\n";  
    }
    else {
      return "fs(\"Wow ... I'm not sure I totally understood you.\",[$files]).\n";
    }
  }
  if($arg[1] eq "finddocs"){
    my $service = $arg[2];
    my $urn = $urns{$service}; 
    my $fieldname = $arg[3];
    my $searchterm = $arg[4];
    @origcontext = @arg;
    splice(@origcontext, 0,5);
    $files = join(",", @origcontext);
    @currcontext = (); 
    my $os = $urns{"os"}; 

    $string = "Select ?Y, ?Z from $files Where (?X, <DocType:$fieldname>, ?Z), (?X,<OS:Name>,?Y) AND ?Z LIKE '%$searchterm%' AND ?Y NOTLIKE '~~~' using DocType for <$urn>, OS for <$os>";
    $query = uri_escape($string); 
    #print "Query? WTF?\n ";
    #print "String is: ", $string, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    ($whatstring) = $urn =~ m:.*/([a-zA-Z0-9\._-]+)[#/]?$:;
    #print "Whatstring is $whatstring\n";
    if($result ne ""){
      @atts = ();
      #print "Result is a ", ref($result->{"rdf:Description"}), "\n";
      if(ref($result->{"rdf:Description"}) eq "ARRAY"){
        foreach $x (@{$result->{"rdf:Description"}}){
          $att = $x->{"$whatstring:$fieldname"};
          $filename = $x->{"OperatingSystem:Name"};
          push(@currcontext, $x->{"rdf:about"}); 
          push(@atts, "$filename has $fieldname: $att");	
        }
      }
      else {
        
        %x = %{$result->{"rdf:Description"}};
        #print "Keys are: ", join(",", keys(%x)), "\n";
        $att = $x{"$whatstring:$fieldname"};
        $filename = $x{"OperatingSystem:Name"};
        push(@currcontext, $x{"rdf:about"}); 
        push(@atts, "$filename has $fieldname: $att");
      }
      return "fs(\"There are ".($#atts+1)." such documents. ". join(", ", @atts), ".\",[".join(",",@currcontext)."]).\n";  
    }
    else {
      return "fs(\"Wow ... I'm not sure I totally understood you.\",[$files]).\n";
    }
  }

    
  if($arg[1] eq "count"){
    $what = $arg[2]; 
    @origcontext = @arg;
    splice(@origcontext, 0,2);
    $files = join(",", @origcontext);
    $string; 
    if($what eq "all"){
      $string = "Select Count from * Where (?X, <OS:Type>, ?Z) using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";
    }
    else {
      $string = "Select COUNT From * Where (?X, <OS:Type>, ?Y) AND ?Y='$what'  using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";

    }
      $query = uri_escape($string); 
    #print "String is: ", $query, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
      $result = get_from_fs($getstring); 
      if($result ne ""){
	#print Dumper($result); 
	$count = $result->{"rdf:Description"}->{"DocumentsCount"}; 
	if($what eq "all"){
	  return "fs(\"You have $count documents.\",[$files]).\n"; 
	}
	else {
	  return "fs(\"You have $count $what documents.\",[$files]).\n"; 
	  }
      }
      else {
	return "fs(\"Sorry. I couldn't connect to FileSphere.\",[$files]).\n"; 
      }
  }
  if($arg[1] eq "list"){
    $what = $arg[2]; 
    @origcontext = @arg;
    splice(@origcontext, 0,3);
    $files = join(",", @origcontext);
    $string = "Select ?Z From * Where (?X, <OS:Type>, ?Y),(?X, <OS:Name>, ?Z) AND ?Y='$what'  using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";
    
    $query = uri_escape($string); 
    #print "List? WTF? \n"; 
    #print "String is: ", $query, "\n";
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    if($result ne ""){
      #print Dumper($result); 
      #$count = $result->{"rdf:Description"}->{"DocumentsCount"}; 
      @schemas = (); 
      @currcontext = (); 
      foreach $x (@{$result->{"rdf:Description"}}){
	push(@schemas, $x->{"OperatingSystem:Name"});
	push(@currcontext, $x->{"rdf:about"}); 
      }
      #print "Current context is: ", join(" ", @currcontext); 
      @schemas = grep(s/\.\w{1,3}$//, @schemas); 
      @schemascard = (); 
      for($i=0; $i <= $#schemas; $i++){
	push(@schemascard, "The ".$card[$i+1]. " is: ".$schemas[$i]); 
      }
      return "fs(\"You have ".($#schemas+1)." documents. ".join(", ", @schemascard)."\",[".join(",",@currcontext)."]).\n";
    }
    else {
      return "fs(\"Sorry. I couldn't connect to FileSphere.\",[$files]).\n"; 
    }
  }
  if($arg[1] eq "filespresent"){
    @origcontext = @arg;
    splice(@origcontext, 0,2);
    $files = join(",", @origcontext);
    $string = "Select Distinct ?Z From * Where (?X, <OS:Type>, ?Z) using OS for <http://www.Neurocom.com.au/EDMS/Schemas/OperatingSystem#>";
    $query = uri_escape($string); 
    $getstring = "Documents_Search?rdql=$query&contains="; 
    $result = get_from_fs($getstring); 
    if($result ne ""){
      #print Dumper($result); 
      #$count = $result->{"rdf:Description"}->{"DocumentsCount"}; 
      @schemas = (); 
      foreach $x (@{$result->{"rdf:Description"}}){
	push(@schemas, $x->{"OperatingSystem:Type"});
	
      }
      return "fs(\"You have documents of the following types: ", join(", ", @schemas), "\",[$files]).\n";
    }
    else {
      return "fs(\"Sorry. I couldn't connect to FileSphere.\",[$files]).\n"; 
      }
  }
  
  if($arg[1] eq "info"){
      #print "Hahaha!\n"; 
    $docid = $arg[2];
    @origcontext = @arg;
    splice(@origcontext, 0,2);
    $files = join(",", @origcontext);      
    $getstring = "Documents_Get?DocID=$docid"; 
    $result = get_from_fs($getstring); 
    if($result ne ""){
      my $serxml = $result->{"metadata"}->{"SerXml"};
      $serxml =~ s/utf-16/utf-8/g; 
      $serxml =~ s/&lt;/</g; 
      $serxml =~ s/&gt;/>/g; 
      #print "SerXML is: $serxml \n"; 
      my $newref = XMLin($serxml);
      #print "Doc description is: ", Dumper($newref);
      @schemas = (); 
      #print "Newref is: ", Dumper($newref);
      %props = %{$newref->{"rdf:Description"}};
      return "fs(\"It is called ".$props{"OperatingSystem:Name"}.", it is ".$props{"OperatingSystem:Size"}." bytes in size, its status is ".$statustypes[$props{"EDMS:Status"}]." and its distribution is ".$props{"FileSphere:Distribution"}.".\", [$files]).\n"; 
      }
      else {
	return "fs(\"Sorry. I couldn't connect to FileSphere.\",[$files]).\n"; 
      }
    }
  }

sub handle_email {
  my @arg = @_;
  if($arg[1] eq "list"){
    $pop = new Mail::POP3Client("inca", "CRCincaSIT", "pop3.cse.unsw.edu.au"); 
    $tabledata = "+TDATA ".$pop->Count.",2"; 
    $rv = "You have ".$pop->Count()." messages. "; 
    for($i=1; $i <= $pop->Count(); $i++){
      foreach ($pop->Head($i)) {
	if(/^From:(.*)$/){
	  $from = $1; 
	}
	if(/^Subject:(.*)$/){
	  $subject = $1; 
	}
      }
      $from =~ s/<[^>]*>//;
      $from =~ s/\"//g; 
      $rv .= "The ".card($i)." is from $from about $subject; ";
      $tabledata.=",$from,$subject"; 
    }
    $pop->Close(); 
    $tabledata .= "+";
    
    $rv .= "\".\n"; 
    return "\"".$tabledata.$rv; 

  }
  if($arg[1] eq "readmsg"){
    $num = $arg[2]; 
    $pop = new Mail::POP3Client("inca", "CRCincaSIT", "pop3.cse.unsw.edu.au"); 
    $count = $pop->Count(); 
    if($count < $num){
      $message = "There are only $count messages."
    }
    else{
      $message = "It reads: ".$pop->Body($num); 
    }
    $pop->Close(); 
    $message =~ s/\"//g; 
    $message =~ s/\n/ /g;
    $message =~ s/\r//g;
    $rv = "\" $message\". \n"; 
    return $rv; 
  }
  
}

sub handle_news {
   my @arg = @_; 
   #print "a0 = $arg[0] a1 = $arg[1]\n"; 
   #print "site = >$site<\n";
   $url = $source{$arg[1]};
   #print "url = $url\n"; 
   open(NEWS, "lwp-request -p http://www-proxy.cse.unsw.edu.au:3128/ $url |") || return "\"$site News is not currently available.\".\n";
  $content = join("", <NEWS>); 
   close(NEWS);
   #$content = get($url); 
   if($content eq ""){
     return "\"$site news is not currently available.\".\n";
   }
   $rss = new XML::RSS;
   $rss->parse($content); 
   $rv = "";
   @items = @{$rss->{'items'}};
   @items = splice(@items, 0, 4); 
   foreach $item (@items) {
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
      -> proxy('http://services.xmethods.net:80/soap', proxy => ['http' => 'http://www-proxy.cse.unsw.edu.au:3128'])
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
  elsif($language eq "italian"){
    $tm = "en_it"; 
  }
  else {
    return "\"Sorry, I can't translate into $language.\".\n";
  }
  $phrase = join(" ", @arg); 
  $response =  SOAP::Lite
    -> uri("urn:xmethodsBabelFish")
      -> proxy('http://services.xmethods.net:80/perl/soaplite.cgi', proxy => ['http' => 'http://www-proxy.cse.unsw.edu.au:3128'])
	-> BabelFish($tm, $phrase); 

  return "\"It wasn't available. I'm sorry.\".\n"  if $response->faultcode; 
  $rv = $response->result; 
  $rv =~ s/[[^:ascii:]]+//g;
  return "\"I think it's roughly: ".$rv."\".\n"; 

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
    return "\"I've noted your appointment for $desc on $day $month.\".\n"; 
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
      $rv .= "[".$appt{"UID"}.", $date, $start, $end, \"".$appt{"SUMMARY"}."\"], "; 
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

sub card {
  my $num = shift(@_); 
  if($num <= 12){
    return $card[$num];
  }
  else {
    return $num;
  }
}
