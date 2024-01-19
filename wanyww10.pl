#!/usr/bin/perl

# A silly little script that takes NOAA western US summaries
# and emits them in 10 character lines.

$latch = 0;
print "USWestWeather\n";
while(<>) {
	chomp; chomp;
	if (/[AP]M [PMCE][SD]T/) {
		# 300 PM PST SUN JAN 14 2024
		(@k) = split(/\s+/, $_);
		print STDOUT "$k[3] $k[4] $k[5] $k[0] $k[1] $k[2]\n\n";
		next;
	}
	if (/^CITY/) { $latch++; next; }
	if ($latch && /^\s*$/) { $latch++; next; }
	next if ($latch < 2);
	next if (/\$\$/);

	# 1234567891123456789211234512345123456789012345
	# 0123456789112345678921234567893123456789412345
	#ALBUQUERQUE              53   12      WINDY    |
	$city = substr($_, 0, 21);
	$city =~ s/\s+$//;
	$f = 0+substr($_, 22, 5);
	$c = 0+substr($_, 27, 5);
	$cond = substr($_, 32,15);
	$cond =~ s/\s+$//;
	$cond =~ s/^\s+//;
	$cond = " $cond\n" if length($cond);
	
	if ($f != 0 || $c != 0) { # impossible to have 0 F = 0 C
		print STDOUT "$city\n${cond}${f}F ${c}C\n";
	} else {
		print STDOUT "$city\nNO DATA\n";
	}
}

