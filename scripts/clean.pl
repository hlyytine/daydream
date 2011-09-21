#!/usr/bin/perl

die "clean.pl <filename>\n" if (!@ARGV);

open(IN, sprintf("<%s", $ARGV[0])) || die "$!";

while (<IN>) {
    chomp;
    $fname=$+ if (/^(\S+)\.\S*\s/);
    push @{%files->{$fname}}, $_;
}    

@fnames=sort {uc($b) cmp uc($a)} keys %files;

for (my $i=0; $i<@fnames; $i++) {
    while ($i<$#fnames&&!($fnames[$i] cmp ++(my $foo=$fnames[$i+1]))) {
        $#{%files->{$fnames[$i+1]}}=0;
        $i++;
    }
}

close IN;
rename($ARGV[0], sprintf("%s~", $ARGV[0]))==1 || die "$!";
open(IN, sprintf("<%s~", $ARGV[0])) || die "$!";
open(OUT, sprintf(">%s", $ARGV[0])) || die "$!";

while (<IN>) {
    chomp;
    if ($_=~/^(\S+)\.\S*\s/) {
        $fname=$+;
	foreach $line (@{%files->{$fname}}) {
	    print OUT $line, "\n";
	}
    }
}    
