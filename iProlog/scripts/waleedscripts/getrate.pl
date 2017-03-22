#!/usr/bin/perl -w
use SOAP::Lite;
$response =  SOAP::Lite 
       -> uri('urn:xmethods-CurrencyExchange')
       -> proxy('http://services.xmethods.net:80/soap', proxy => ['http' => 'http://web-2.cse.unsw.edu.au:3128'])
       -> getRate("australia", "japan"); 

die "Fault: ".$response->faultcode." ".$response->faultdetail." ".$response->faultstring if $response->faultcode; 
print "response is: ", $response->result, "\n"; 

$response =  SOAP::Lite 
       -> uri('urn:ShakesIntf-IShakespeare')
       -> proxy('http://www.nickhodges.com/bin/ShakespeareWS.exe/soap/IShakespeare', proxy => ['http' => 'http://web-2.cse.unsw.edu.au:3128'])
       -> GetShakespeareInsult(); 

die "Fault: ".$response->faultcode." ".$response->faultdetail." ".$response->faultstring if $response->faultcode; 
print "response is: ", $response->result, "\n"; 

$response =  SOAP::Lite 
       -> uri("urn:xmethodsBabelFish")
       -> proxy('http://services.xmethods.net:80/perl/soaplite.cgi', proxy => ['http' => 'http://web-2.cse.unsw.edu.au:3128'])
       -> BabelFish("en_fr", "Hello my friend. I am crazy."); 

die "Fault: ".$response->faultcode." ".$response->faultdetail." ".$response->faultstring if $response->faultcode; 
print "response is: ", $response->result, "\n"; 
