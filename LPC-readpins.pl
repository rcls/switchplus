#!/usr/bin/perl

use strict;
use warnings;

my $symbol = qr{\b[A-Z][0-9A-Z_/]+|-};

my $pin = qr/\bx\b|\b[A-Z]?[1-9]\d*\b,?|[-,]/;

my $notes = qr/(?: +\[[1-9]\d*\])?/;

my $reset = qr/\b[IO];(?: *F\b| *IA\b| *P[UD]\b)?|-?/;

my $reset2 = qr/(?:\bP[UD]\b)?/;

my $type = qr{(?: +(?:\bI/O\b|\b[IO]\b|-))?};

my $func = qr/[A-Z0-9_]+(?:\[\d+\])?\s*--/i;

my $in_table3;
my $item_pins;

'CLK0' =~ /^$symbol$/  or  die;

while (<>) {
    $in_table3 = 1  if  /^Table 3\b/;
    $in_table3 = undef  if  /^Objective data sheet/;

    next unless $in_table3;

    if (/^($symbol)((?: +$pin){6})$notes +($reset)($type) +($func?).*/) {
        my $s = $1;
        my $p = $2;
        my $r = $3;
        my $t = $4;
        my $f = $5;

        $p =~ s/^ +//;
        $p =~ s/ +$//;
        $p =~ s, +,/,g;
        $r =~ s/ +//g;
        $t =~ s/^ +//;

        print "SYMBOL=$s PINS=$p RESET=$r TYPE=$t FUNC=$f\n";
        $item_pins = 1;
        next;
    }

    if (/^ {20,}($reset2?)($type) *($func).*/) {
        my $r = $1;
        my $t = $2;
        my $f = $3;
        $t =~ s/^ +//;
        print " RESET2=$r TYPE=$t FUNC=$f\n";
        $item_pins = undef;
        next;
    }

    if ($item_pins  and  /^ {9,}((?: +$pin)+)/) {
        my $p = $1;
        $p =~ s/^ +//;
        $p =~ s, +,/,g;
        print " PINS=$p\n";
        next;
    }

    $item_pins = undef;
}
