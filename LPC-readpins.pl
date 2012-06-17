#!/usr/bin/perl

use strict;
use warnings;

my $verb = shift @ARGV;

# Regexes for scraping the pdftotext output...
my $symbol = qr{\b[A-Z][0-9A-Z_/]+|-};
my $pin = qr/\bx\b|\b[A-Z]?[1-9]\d*\b,?|[-,]/;
my $notes = qr/(?: +\[[1-9]\d*\])?/;
my $reset = qr/\b[IO];(?: *F\b| *IA\b| *P[UD]\b)?|-?/;
my $reset2 = qr/(?:\bP[UD]\b)?/;
my $type = qr{(?: +(?:\bI/O\b|\b[IO]\b|-))?};
my $func = qr{[A-Z0-9_/]+(?:\[\d+\])?(?:\s*\([A-Z_]+\))?\s*--}i;

# Flag for the pin table.
my $in_table3;

# Current item.
my $pin_item;

# All items.
my @pins;

# Which EMC channel are we going to use?
my $emc_channel = 0;

$\ = "\n";

sub pin_start($$$$$)
{
    $_[4] =~ s/ *\(.*\)//;
    $_[0] =~ s/^-$/NC/;
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
    return  unless  $pin_item;
    $pin_item->{symbol} =~ s|ADC0_(\d+)/ADC1_\1|ADC_$1|;
    $pin_item->{funcs}[0]{func} = $pin_item->{symbol}  if
        $pin_item->{funcs}[0]{func} eq '';

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


sub pin_func(@)
{
    $_[2] =~ s/ *\(.*\)//;
    $pin_item->{reset} .= $_[0];
    push @{$pin_item->{funcs}}, { type => $_[1], func => $_[2] };
}


while (<>) {
    $in_table3 = 1  if  /^Table 3\b/;
    $in_table3 = undef  if  /^Objective data sheet/;

    next unless $in_table3;

    next  if  /^LPC4350_30_20_10/;

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

    if ($pin_item  and  m{^([\w/]+)( {10}.*)?$}) {
        $pin_item->{symbol} .= $1;
    }

    die  if  $pin_item  and /--/;

    next  if  /^$/;
    next  if  / {20}/;

    pin_eject;
}


sub pin_dump($)
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


if ($verb eq 'dump') {
    pin_dump $_  for  @pins;
    exit 0;
}


my %wanted_functions;

for my $p (@pins) {
    for (map { $_->{func} } @{$p->{funcs}}) {
        if (/^ENET_/) {
#            next  if /^ENET_RX_ER/;
            $wanted_functions{$_} = 1;
        }
        elsif (/^LCD_/) {
            $wanted_functions{$_} = 1;
        }
        elsif (/^USB0_/) {
            next  if  /ULPI/;
            next  if  /PWR_FAULT/;
            next  if  /PPWR/;
            next  if  /USB1_VBUS/;
            next  if  /USB1_IND/;
            $wanted_functions{$_} = 1;
        }
        elsif (/^EMC_/) {
            next  if /^EMC_D(\d+)$/  and  $1 > 15;
            next  if /^EMC_A(\d+)$/  and  $1 > 14;
            next  if /^EMC_DYCS(\d+)$/  and  $1 ne $emc_channel;
            next  if /^EMC_BLS/; # Just DQM is wanted.
            next  if /^EMC_DQMOUT[^01]/;
            next  if /^EMC_CS/; # We just want DYCS, right?
            next  if /^EMC_OE/;
            next  if /^EMC_CLK(.*)/  and  $1 ne $emc_channel;
            next  if /^EMC_CKEOUT(.*)/  and  $1 ne $emc_channel;
            $wanted_functions{$_} = 1;
        }
        elsif (/^SPIFI_/) {
            $wanted_functions{$_} = 1;
        }
        elsif (/^SSP1_/) {
            $wanted_functions{$_} = 1;
        }
        elsif (/^I2C/) {
            $wanted_functions{$_} = 1;
        }
#        elsif (/^U[0]_(RX|TX)/) {
#            $wanted_functions{$_} = 1;
#        }
        elsif (/^U3/ and /RXD|TXD|RTS|CTS/) {
            $wanted_functions{$_} = 1;
        }
    }
}

my %assigned_funcpins;
my %assigned_pinfuncs;

my %pinfuncs;
my %funcpins;

for my $p (@pins) {
    my $pinlist = $p->{pins}[0];
    $pinlist =~ s,/.*,,;
    next  if  $pinlist eq '-';
    my @pinlist = split /,/, $pinlist;
    die $p->{symbol}  if  @pinlist > 1  and  @{$p->{funcs}} > 1;
    for my $f (@{$p->{funcs}}) {
        push @{$pinfuncs{$_}}, $f->{func}  for  @pinlist;
        push @{$funcpins{$f->{func}}}, $_  for  @pinlist;
    }
}


#print "Keys pinfuncs\n";
#print join ' ', keys %pinfuncs;
#print "\n";

#print "Values pinfuncs\n";
#print join (' ', @{$_}), "\n"  for  values %pinfuncs;
#print "\n";

#print "Keys funcpins\n";
#print join ' ', keys %funcpins;
#print "\n";

#print "Values funcpins\n";
#print join (' ', @{$_}), "\n"  for  values %funcpins;
#print "\n";

sub assign($$)
{
    my ($p, $f) = @_;
    die "$f"  if  not exists $funcpins{$f};
    die "$p"  if  not exists $pinfuncs{$p};
    die "$p $f"  if  not grep { $_ eq $f } @{$pinfuncs{$p}};
    die "$p $f "  if  exists $assigned_pinfuncs{$p};
    $assigned_pinfuncs{$p} = $f;
    $assigned_funcpins{$f} = $p;
#    print "$p $f\n";
}


# If a pin has only one function - assign it.
# If a pin has only one unassigned function - assign it.
# If a pin has only one wanted, unassigned function - assign it.
# If a wanted_functions function only has one possible pin - assign it.

sub assign_pins_with_one_function()
{
    print STDERR '# assign_pins_with_one_function';
    for (keys %pinfuncs) {
        assign $_, $pinfuncs{$_}[0]  if  @{$pinfuncs{$_}} == 1;
    }
}

sub assign_pins_with_one_unassigned_function()
{
    print STDERR '# assign_pins_with_one_unassigned_function';
    for (keys %pinfuncs) {
        next  if  exists  $assigned_pinfuncs{$_};
        my @unass = grep { not exists $assigned_funcpins{$_} } @{$pinfuncs{$_}};
        # Don't assign R and GPIO.
        @unass = grep { ! /^R$/  and  ! /^S?GPIO/ } @unass  if  @unass > 1;
        # If we have more than one unassigned function, just select the wanted
        # ones.
        @unass = grep { exists $wanted_functions{$_} } @unass
            if  @unass > 1;
        assign $_, $unass[0]  if  @unass == 1;
    }
}

sub assign_functions_with_one_pin()
{
    print STDERR '# assign_functions_with_one_pin';
    for (keys %wanted_functions) {
        next  if  exists  $assigned_funcpins{$_};

        my @unass = grep { not exists $assigned_pinfuncs{$_} } @{$funcpins{$_}};
        assign $unass[0], $_  if  @unass == 1;
    }
}


assign_pins_with_one_function;

# FIXME - check these.
assign 'C14', 'LCD_VD7';
assign 'A12', 'LCD_VD14';
assign 'B11', 'LCD_VD15';
assign 'C8',  'LCD_VD16';

assign 'E13', 'I2C1_SCL';
assign 'G14', 'I2C1_SDA';

assign 'M6',  'ENET_RXD2';
assign 'N8',  'ENET_RXD3';
assign 'N10', 'ENET_TXD2';
assign 'M9',  'ENET_TXD3';
assign 'M2',  'ENET_TX_EN';
assign 'N4',  'ENET_RX_DV';
assign 'T1',  'ENET_CRS';
assign 'E4',  'ENET_MDC';
#assign 'M11', 'ENET_TX_CLK';
# FIXME - one of these is unneeded.  Actually, probably don't need either.
assign 'N1',  'ENET_TX_ER';
assign 'N6',  'ENET_RX_ER';

assign 'E7',  'SSP1_MISO';
assign 'B7',  'SSP1_MOSI';
assign 'E9',  'SSP1_SSEL';

assign 'K14', 'USB0_IND0';
assign 'J13', 'USB0_IND1';

#assign 'D16', 'U1_TXD';
#assign 'K15', 'U2_RXD';

#assign 'F6', 'U1_CTS';
#assign 'F5', 'U1_RTS';
#assign 'F15', 'U1_CTS';
#assign 'N16', 'U1_RTS';

assign 'K11', 'U3_RXD';
assign 'J12', 'U3_TXD';

while (1) {
    my $start = keys %assigned_pinfuncs;

    print STDERR "# Done pins $start";

#    assign_pins_with_one_unassigned_function;
    assign_functions_with_one_pin;

    my $end = keys %assigned_pinfuncs;
    print STDERR "# Just did ", $end - $start, " pins";
    last  if  $start == $end;
}


print STDERR "# Done a total of ", scalar (keys %assigned_pinfuncs), " pins";

for (sort keys %wanted_functions) {
    next  if  exists $assigned_funcpins{$_};
    print STDERR "# Unassigned function $_ ", join (
        ' ',
        grep { not exists $assigned_pinfuncs{$_} } @{$funcpins{$_}});
}

# If we've assigned all wanted functions, default assign the unwanted pins...
unless (grep { not exists $assigned_funcpins{$_} } keys %wanted_functions) {
    for my $pp (@pins) {
        my $p = $pp->{pins}[0];
        next  if  exists $assigned_pinfuncs{$p};
        my @f = grep { $_->{func} =~ /^GPIO/ } @{$pp->{funcs}};
        assign $p, $f[0]{func}  if  @f == 1;
    }
}

# Now assign some leftovers.
assign 'D12', 'GP_CLKIN';
assign 'D14', 'SD_CLK';
assign 'L1', 'CGU_OUT0';
assign 'L12', 'CGU_OUT1';
assign 'T10', 'CLKOUT';
assign 'M12', 'I2S0_RX_MCLK';
assign 'F13', 'I2S0_TX_SCK';
assign 'P12', 'I2S1_RX_SCK';

my @rows = split //, 'ABCDEFGHJKLMNPRT';
my @cols = 1..16;

if ($verb eq 'list') {
    for my $r (@rows) {
        $assigned_pinfuncs{"$r$_"}  and
            print "$r$_ ", $assigned_pinfuncs{"$r$_"}  for  @cols;
    }
    exit 0;
}

my %colors = (
    'VBAT' => '#ffbbbb',
    'VDD' => '#ffbbbb',
    'VPP' => '#ffbbbb',
    'VSS' => '#cccccc',
    'ENET' => '#ffeecc',
    'LCD' => '#ccddff',
    'EMC' => '#ccffcc',
    'USB' => '#ffccff',
    'SPIFI' => '#ffffcc',
    'I2C' => '#ccffff',
    'U' => '#8888ff',
    'SSP' => '#ccffff'
    );

print '<html>';
print '<head><title>PINS</title></head>';
print "<style type='text/css'>";
print ".$_ { background: $colors{$_}; }"  for  sort keys %colors;
print '.TDI, .TDO, .TCK, .TMS, .TRST, .DBGEN, .RESET, .XTAL { font-weight: bold; }';
print '.GPIO, .SGPIO { color: #aaaaaa; }';
print '.SGPIO { text-decoration: underline; }';
print '</style>';
print '<body>';
print '<table>';
print "<tr><th />", map { "<th>$_</th>" } @cols;
print "</tr>";
for my $r (@rows) {
    print "<tr><th>$r</th>";
    for my $c (@cols) {
        my $text = $assigned_pinfuncs{"$r$c"} // "";
        $text =~ /.*(VSS|VDD|^I2C|^[A-Z]*)/;
        my $class = $1;
        unless ($text =~ s|/|<br />|g
                or  $text =~ s|^WAKEUP|WAKE</br>UP|
                or  $text =~ s|^GPIO(\d+)\[(\d+)\]$|GPIO<br />$1_$2|
                or  $text =~ s|^SGPIO(\d+)$|SGPIO<br />$1|
                or  $text =~ s|^(V[SD][SD])(.+)$|$1<br />$2|) {
            $text =~ s|_(..)|<br />$1|;
            $text =~ s|_(....)|<br />$1|;
            $text =~ s|(....)_|$1<br />|;
        }
        $class = 'SGPIO'
            if  $class eq 'GPIO'  and  grep { /^SGPIO/ } @{$pinfuncs{"$r$c"}};
        print "<td class='$class'>$text</td>";
    }
    print "<th>$r</th>";
    print '</tr>';
}
print "<tr><th />", map { "<th>$_</th>" } @cols;
print "</tr>";
print "</table>\n";
print "</body></html>\n";

exit 0;
