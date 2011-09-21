@monthnames=("Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec");

while(<>) {
    if (/^ /) {
	print $_;
    } else {
	printf ("%12.12s ",$_); $flags=substr($_,35,4);
	print $flags;
	$size=substr($_,39,8);
	print $size;
	$mon=substr($_,52,3);
	for ($i=0;$i<12;$i++) { if ($monthnames[$i] eq $mon) { 
	    $month=$i+1; }
	}
	$day=substr($_,56,2);
	$year=substr($_,70,2);
	printf (" %2.2d.%2.2d.%2.2d ",$day,$month,$year);
	$_=<>;
	s/^\s+//;
	print $_;
    }
}
