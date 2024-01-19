#!/usr/bin/perl

# A silly little script that takes NOAA international weather summaries
# and emits them in 10 character lines.

$latch = 0;
print "WorldWeather\n";
while(<>) {
	chomp; chomp;
	if (/UTC/) {
		# 1200 UTC SUN JAN 14 2024
		(@k) = split(/\s+/, $_);
		print STDOUT "$k[3] $k[4] $k[5]\n\n";
		next;
	}
	if (/^CITY/) { $latch++; next; }
	if ($latch && /^\s*$/) { $latch++; next; }
	next if ($latch < 2);
	next if (/\$\$/);

	# 1234567890123456712345678901234561231234
	# 0123456789112345678921234567893123456789
	# ABERDEEN         WINDY     NOON   34   1
	$city = substr($_, 0, 17);
	$city =~ s/\s+$//;
	$cond = substr($_, 17,10);
	$cond =~ s/\s+$//;
	$cond = " $cond\n" if length($cond);
	$temps = substr($_, 32);
	$temps =~ s/^\s+//;
	($f, $c) = split(/\s+/, $temps);
	
	if ($f ne "NO") {
		print STDOUT "$city\n${cond}${f}F ${c}C\n";
	} else {
		print STDOUT "$city\nNO DATA\n";
	}
}

