#!/usr/bin/perl

# init translation tables (digits are done in place)
@A_table = qw(
 0x06 0x0c 0x0e 0x12 0x14 0x1a 0x1c 0x1e 0x20 0x24 0x26 0x28 0x2a 0x2c 0x2e 0x33 0x35 0x37 0x39 0x3c 0x3e 0x43 0x45 0x47 0x49 0x4b
);
@a_table = qw(
 0x08 0x0d 0x10 0x13 0x16 0x1b 0x1d 0x1f 0x21 0x25 0x27 0x29 0x2b 0x2d 0x30 0x34 0x36 0x38 0x3a 0x3d 0x40 0x44 0x46 0x48 0x4a 0x4c
);

@etc_keys = qw(
 ` ~ ! @ # $ % & * ( ) - _ = + [ { ] } \ | ; : ' " , < . > / ?
);
@etc_values = qw(
 0x02 0x72 0x4e 0x4d 0x77 0x5a 0x5b 0x5c 0x60 0x5e 0x5f 0x66 0x6e 0x67 0x61 0x6a 0x6f 0x6c 0x71 0x68 0x70 0x65 0x64 0x5d 0x4f 0x62 0x78 0x63 0x79 0x70 0x69
);
foreach(@etc_keys) { $etc{$_} = shift(@etc_values); }
$etc{' '} = "0x05";

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
			$kk = "0x01";
		} elsif ($k eq 13) {
			next;
		} elsif ($k eq 9) {
			$kk = "0x05";
		} elsif ($k >= $__A && $k <= $__Z) {
			$kk = $A_table[$k - $__A];
		} elsif ($k >= $__a && $k <= $__z) {
			$kk = $a_table[$k - $__a];
		} elsif ($k >= $__0 && $k <= $__9) {
			$kk = sprintf("0x%02x", 80 + ($k - $__0));
		} elsif (defined($etc{$w})) {
			$kk = $etc{$w};
		} else {
			die("can't encode character: $w\n");
		}
		$nurec .= "$kk, ";
		$tchars++;
		if (++$chars == 128) {
			# 6 times 128 = 768
			die("assertion failed: @{[ length($nurec) ]}\n$nurec\n")
				if (length($nurec) != 768);
			push(@records, $nurec);
			$nurec = '';
			$chars = 0;
		}
	}
}

# add terminal null if there isn't one (may require a whole packet)
$nurec .= "0x00, " x (128 - $chars);
die("assertion 2 failed: ($chars) @{[ length($nurec) ]}\n$nurec\n")
	if (length($nurec) != 768);
push(@records, $nurec);

$g1 = int($tchars / 256);
$g2 = $tchars & 255;
$g3 = int(scalar(@records) / 256);
$g4 = (scalar(@records) & 255);
printf "usend(10,0xaa,0x55,0x00,0x00,0x%02x,0x%02x,0x00,0x%02x,0x%02x,0x%02x);\n",
	$g1, $g2, $g3, $g4,
	(170+85+$g1+$g2+$g3+$g4) & 255;
print "uwait(0xaa);\n";

foreach(@records) {
	$j = 0;
	map { $j += hex($_) } split(/, /, $_);
	print "usend(129,\n$_\n@{[ $j & 255 ]});\n";
	print "uwait(0xaa);\n";
}

print <<"EOF";
usend(10, 0xAA, 0x55, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE);
uwait(0xAA);
EOF

__END__

/* record separator: 0x01
 * line separator: 0x02 (`)
 * end of record: 0x00
 * 0-9: 0x50 - 0x59
 *  A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
 * 060c0e12141a1c1e202426282a2c2e333537393c3e434547494b
 *  a b c d e f g h i j k l m n o p q r s t u v w x y z
 * 080d1013161b1d1f212527292b2d303436383a3d404446484a4c
 *  ` ~ ! @ # $ % ` & * ( ) - _ = + [ { ] } \ | ; : ' " , < . > / ?
 * 02724e4d775a5b025c605e5f666e67616a6f6c71687065645d4f627863797069
 */
