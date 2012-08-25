#!/usr/bin/perl

use strict;
use warnings;

die "Usage: $0 <pins file> <symbol>"  unless  @ARGV == 2;

open my $PINS, '<', $ARGV[0]  or  die;

my %pins;
my %labels;
my %label_count;

my $pinre = qr/(?:[A-Z]*\d+)/;

while (<$PINS>) {
    chomp;
    die  unless  /^($pinre(?:,$pinre)*) (.*)/;

    my $pins = $1;
    my $label = $2;

    for (split /,/, $pins) {
        $pins{$_} = $label;
        $labels{$label} = $_;
        ++$label_count{$label};
    }
}

close $PINS  or  die;


for (keys %label_count) {
    delete  $labels{$_}  if  $label_count{$_} > 1;
}

open my $SYMBOL, '<', $ARGV[1]  or  die;

my $text = '';

while (<$SYMBOL>) {
    $text .= $_;

    next  unless  /}/  and  $text =~ /^pin/m;

    $text =~ /^pinlabel=(.*)/m  or  die;
    my $label = $1;

    $text =~ /^pinseq=(.*)/m  or  die;
    my $seq = $1;

    $text =~ /^pinnumber=(.*)/m  or  die;
    my $number = $1;

    $seq = $labels{$label}  if
        $seq eq '0'  and  $label ne 'unknown'  and  exists $labels{$label};

    $number = $labels{$label}  if
        $number  eq '0'  and  $label ne 'unknown'  and  exists $labels{$label};

    $seq = $number  if  $seq eq '0';
    $number = $seq  if  $number eq '0';

    $label = $pins{$seq}  if
        $label eq 'unknown'  and  $seq ne '0'  and  exists $pins{$seq};

    die  "seq $seq v. number $number"  if  $seq ne $number;

    die  "$label 0"  if  $number  eq  '0';
    die  $seq unless  exists $pins{$seq};
    die  "$label v. $pins{$seq}"  if  $label  ne  $pins{$seq};
    die  'unknown'  if  $label  eq  'unknown';

    die  "$seq v. $label"  if
        $seq ne '0'  and  $label ne 'unknown'  and  exists $pins{seq}  and
        $label ne $pins{seq};

    die  "$seq v. $label"  if
        $seq ne '0'  and  $label ne 'unknown'  and  exists $pins{seq}  and
        $seq ne $labels{label};

    $text =~ s/^pinseq=.*/pinseq=$seq/m;
    $text =~ s/^pinnumber=.*/pinnumber=$number/m;
    $text =~ s/^pinlabel=.*/pinlabel=$label/m;
#    print "BLOCK.... $seq $number $label\n";
    print $text;
    $text = '';
}

#print "TAIL....\n";
print $text;

close $SYMBOL  or  die;
