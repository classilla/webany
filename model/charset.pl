#!/usr/bin/perl
#
# Create an uploadable file with the charset.
# Eight characters per line.
#

# Index: charset
#        c h a r s e tLF";
$buf = "101f08383a163d01";

# First line, ignore known control sequences
#         1 2 3 4 5 6 7 8LF
$buf .= "050203040506070801";

# Emit remainder in 8 byte gulps
for($i=8;$i<256;$i+=8) {
	for($j=$i;$j<$i+8;$j++) {
		$buf .= sprintf("%02x", $j);
	}
	$buf .= "01";
}

# Terminal null
$buf .= "00";

# Chop into packets with checksums.
# (Generic code to turn any hex string into transmissible data.)
#
# Add padding
$chars = length($buf) / 2;
$buf .= "00" x (128 - ($chars & 127));
die("wrong length @{[ length($buf) ]}\n") if (length($buf) & 255);

# Generate header and header checksum
$records = length($buf) / 256; # guaranteed integral
$g1 = int($chars / 256);
$g2 = $chars & 255;
$g3 = int($records / 256);
$g4 = $records & 255;
print pack("H*", sprintf "aa550000%02x%02x00%02x%02x%02x",
	$g1, $g2, $g3, $g4,
	(170+85+$g1+$g2+$g3+$g4) & 255);

# Emit packetized data with checksums
for($i=0;$i<length($buf);$i+=256) {
	$subbuf = substr($buf, $i, 256);
	$j = 0;
	map { $j += hex($_) } split(/(..)/, $subbuf);
	$j &= 255;
	print pack("H*", $subbuf . sprintf("%02x", $j));
}

__DATA__

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

