#!/usr/bin/perl

while(<>) {
   /^(\S+)/; $fname = $1; 
   
   print $fname;
   for ($i=35-length($fname);$i;$i--) {
      print " ";
   }
   print "N--- ";
   @hillo=stat($fname);
   printf "%7d ",@hillo[7];
   $tim=gmtime;
   print $tim;
   print "\n                                   ";
   s/\@n/\n                                   /g;
   print substr($_,length($fname)+1);
}
