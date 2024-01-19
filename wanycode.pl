#!/usr/bin/perl
#
# encode strings of text into Web-@nywhere browser indices
# (C)2024 Cameron Kaiser. BSD license.
# All rights reserved.
#

if ($ARGV[1] eq '-h' || $ARGV[1] eq '--help' || $ARGV[1] eq '-help' || $ARGV[1] eq '-?') {
	print <<"EOF";
usage: $0 { files }

converts provided files or standard input to Web-\@nywhere encoding with
checksums for use with wanysend.c. emits on standard output.
(c)2024 cameron kaiser * all rights reserved * BSD license
EOF
	exit(1);
}

# init translation tables (digits are done in place)
@A_table = qw(
 06 0c 0e 12 14 1a 1c 1e 20 24 26 28 2a 2c 2e 33 35 37 39 3c 3e 43 45 47 49 4b
);
@a_table = qw(
 08 0d 10 13 16 1b 1d 1f 21 25 27 29 2b 2d 30 34 36 38 3a 3d 40 44 46 48 4a 4c
);

@etc_keys = qw(
 ` ~ ! @ # $ % & * ( ) - _ = + [ { ] } \ | ; : ' " , < . > / ?
);
@etc_values = qw(
 02 72 4e 4d 77 5a 5b 5c 60 5e 5f 66 6e 67 61 6a 6f 6c 71 68 70 65 64 5d 4f 62 78 63 79 70 69
);
foreach(@etc_keys) { $etc{$_} = shift(@etc_values); }
$etc{' '} = "05";

$__A = ord("A");
$__Z = ord("Z");
$__a = ord("a");
$__z = ord("z");
$__0 = ord("0");
$__9 = ord("9");

@records = ();
$tchars = 1; # count terminal null
$chars = 0;
$nurec = '';

while(<>) {
	foreach $w (split('', $_)) {
		$k = ord($w);
		if ($k eq 10) {
			$kk = "01";
		} elsif ($k eq 13) {
			next;
		} elsif ($k eq 9) {
			$kk = "05";
		} elsif ($k >= $__A && $k <= $__Z) {
			$kk = $A_table[$k - $__A];
		} elsif ($k >= $__a && $k <= $__z) {
			$kk = $a_table[$k - $__a];
		} elsif ($k >= $__0 && $k <= $__9) {
			$kk = sprintf("%02x", 80 + ($k - $__0));
		} elsif (defined($etc{$w})) {
			$kk = $etc{$w};
		} else {
			die("can't encode character: $w\n");
		}
		$nurec .= $kk;
		$tchars++;
		if (++$chars == 128) {
			push(@records, $nurec);
			$nurec = '';
			$chars = 0;
		}
	}
}

# add terminal null if there isn't one (may require a whole packet)
$nurec .= "00" x (128 - $chars);
push(@records, $nurec);

$g1 = int($tchars / 256);
$g2 = $tchars & 255;
$g3 = int(scalar(@records) / 256);
$g4 = (scalar(@records) & 255);
print pack("H*",
	sprintf "aa550000%02x%02x00%02x%02x%02x",
	$g1, $g2, $g3, $g4,
	(170+85+$g1+$g2+$g3+$g4) & 255);

foreach(@records) {
	$j = 0;
	map { $j += hex($_) } split(/(..)/, $_);
	$j &= 255;
	print pack("H*", $_ . sprintf("%02x", $j));
}

__END__

/* record separator: 01
 * line separator: 02 (`)
 * end of record: 00
 * 0-9: 50 - 59
 *  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
 * 060c0e12141a1c1e202426282a2c2e333537393c3e434547494b
 *  a b c d e f g h i j k l m n o p q r s t u v w x y z
 * 080d1013161b1d1f212527292b2d303436383a3d404446484a4c
 *  ` ~ ! @ # $ % ` & * ( ) - _ = + [ { ] } \ | ; : ' " , < . > / ?
 * 02724e4d775a5b025c605e5f666e67616a6f6c71687065645d4f627863797069
 */
