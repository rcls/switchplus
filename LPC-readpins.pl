#!/usr/bin/perl

use strict;
use warnings;

my $symbol = qr{\b[A-Z][0-9A-Z_/]+|-};

my $pin = qr/\bx\b|\b[A-Z]?[1-9]\d*\b,?|[-,]/;

my $notes = qr/(?: +\[[1-9]\d*\])?/;

my $reset = qr/\b[IO];(?: *F\b| *IA\b| *P[UD]\b)?|-?/;

my $reset2 = qr/(?:\bP[UD]\b)?/;

my $type = qr{(?: +(?:\bI/O\b|\b[IO]\b|-))?};

my $func = qr{[A-Z0-9_/]+(?:\[\d+\])?(?:\s*\([A-Z_]+\))?\s*--}i;

my $in_table3;
my $pin_item;

my @pins;


sub pin_start($$$$$)
{
    $pin_item = {
        symbol => $_[0],
        pins => $_[1],
        reset => $_[2],
        funcs => [ { type => $_[3], func => $_[4] } ]
    };
    push @pins, $pin_item;
}


sub pin_eject()
{
    $pin_item->{funcs}[0]{func} = $pin_item->{symbol}  if
        $pin_item  and  $pin_item->{funcs}[0]{func} eq '';

    $pin_item = undef;
}


sub pin_print($)
{
    my $p = $_[0];
    print "SYMBOL=$p->{symbol}";
    print " PINS=", join '/', @{$p->{pins}};
    print " RESET=", $p->{reset};
    print " FUNCS=", scalar(@{$p->{funcs}});
    print "\n";

    for (@{$p->{funcs}}) {
        print " TYPE=", $_->{type};
        print " FUNC=", $_->{func};
        print "\n";
    }

}


sub pin_pins(@)
{
    my @p = @_;

    for (@{$pin_item->{pins}}) {
        last  unless  @p;
        $_ .= shift @p  if  /,$/  or  /\d{3}/  and  $p[0] =~ /^,/;
    }
}


sub pin_func(@)
{
    $pin_item->{reset} .= $_[0];
    push @{$pin_item->{funcs}}, { type => $_[1], func => $_[2] };
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
        $f =~ s/\s*--$//;

        pin_eject;
        pin_start $s, $p, $r, $t, $f;
        next;
    }

    if ($pin_item  and  /^ {20,}($reset2?)($type) *($func).*/) {
        my $r = $1;
        my $t = $2;
        my $f = $3;

        $t =~ s/^ +//;
        $f =~ s/\s*--$//;

        pin_func $r, $t, $f;
        next;
    }

    if ($pin_item  and  /^ {9,}((?: +$pin)+)/) {
        my $p = $1;
        $p =~ s/^ +//;
        pin_pins split / +/, $p;
        next;
    }

    die  if  $pin_item  and /--/;

    next  if  /^$/;
    next  if  / {20}/;

    pin_eject;
}


pin_print $_  for  @pins;
