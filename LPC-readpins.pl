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
my $pin_item;

'CLK0' =~ /^$symbol$/  or  die;

sub pin_start($$$$$)
{
    $pin_item = {
        symbol => $_[0],
        pins => $_[1],
        reset => $_[2],
        type => $_[3],
        func => $_[4]
    }
}

sub pin_eject()
{
    return  unless  $pin_item;
    $pin_item->{pins} = join '/', @{$pin_item->{pins}};
    print
        join ' ',
        map { uc($_) . "=$pin_item->{$_}"} qw/symbol pins reset type func/;
    print "\n";
    $pin_item = undef;
}

sub pin_pins(@)
{
    my @p = @_;

    for (@{$pin_item->{pins}}) {
        last  unless  @p;
        $_ .= shift @p  if  /,$/  or  /\d{3}/  and  $p[0] =~ /^,/;
    }
}


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
        #$p =~ s/ +$//;
        $p = [ split / +/, $p ];
        #$p =~ s, +,/,g;
        $r =~ s/ +//g;
        $t =~ s/^ +//;

        pin_eject;
        pin_start $s, $p, $r, $t, $f;
        next;
    }

    if (/^ {20,}($reset2?)($type) *($func).*/) {
        my $r = $1;
        my $t = $2;
        my $f = $3;
        $t =~ s/^ +//;
        pin_eject;
        print " RESET2=$r TYPE=$t FUNC=$f\n";
        next;
    }

    if ($pin_item  and  /^ {9,}((?: +$pin)+)/) {
        my $p = $1;
        $p =~ s/^ +//;
        pin_pins split / +/, $p;
        next;
    }

    next  if  /^$/;

    pin_eject;
}
