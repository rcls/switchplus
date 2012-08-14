v 20110115 2
C 54900 44100 1 0 0 ksz8895-smii.sym
{
T 55200 49000 5 10 1 1 0 0 1
value=KSZ8895
T 56200 49000 5 10 1 1 0 0 1
refdes=U2
}
C 52700 42700 1 0 0 lpc-eth.sym
{
T 52800 49000 5 10 1 1 0 0 1
value=LPC ENET
T 54000 49000 5 10 1 1 0 0 1
refdes=U1
}
N 54900 47400 54600 47400 4
N 54900 45300 54600 45300 4
N 54900 45000 54600 45000 4
N 54900 46200 54600 46200 4
N 54600 46200 54600 46300 4
N 54900 48600 54600 48600 4
N 54600 48600 54600 48700 4
C 54600 43300 1 0 0 gnd-1.sym
N 54600 43600 54700 43600 4
C 58200 42700 1 0 0 ksz8895-io.sym
{
T 58600 48200 5 10 1 1 0 0 1
value=KSZ8895
T 59600 48200 5 10 1 1 0 0 1
refdes=U2
}
N 54600 43200 55100 43200 4
N 57000 44000 57000 47900 4
N 57000 47900 58300 47900 4
N 58300 47600 57100 47600 4
N 57100 47600 57100 43900 4
N 55200 42900 54600 42900 4
T 60700 43200 9 10 1 0 0 0 2
Float for
normal
T 61100 46700 9 10 1 0 0 0 2
Float for
normal
C 61000 47800 1 0 0 resistor-1.sym
{
T 61300 48200 5 10 0 0 0 0 1
device=RESISTOR
T 61250 48050 5 10 1 1 0 0 1
refdes=R95
T 61000 47800 5 10 0 1 0 0 1
footprint=0603
}
C 61000 47500 1 0 0 resistor-1.sym
{
T 61300 47900 5 10 0 0 0 0 1
device=RESISTOR
T 61300 47300 5 10 1 1 0 0 1
refdes=R45
T 61000 47500 5 10 0 1 0 0 1
footprint=0603
}
C 61700 47900 1 0 0 3.3V-plus-1.sym
N 61900 47900 61900 47600 4
N 61000 47200 61700 47200 4
N 58300 47200 58100 47200 4
N 58300 46800 58100 46800 4
N 58300 46500 58100 46500 4
N 58300 46100 58100 46100 4
C 57400 45600 1 0 0 resistor-1.sym
{
T 57700 46000 5 10 0 0 0 0 1
device=RESISTOR
T 57700 45900 5 10 1 1 0 0 1
refdes=R42
T 57400 45600 5 10 0 1 0 0 1
footprint=0603
}
C 57400 45300 1 0 0 resistor-1.sym
{
T 57700 45700 5 10 0 0 0 0 1
device=RESISTOR
T 57500 45100 5 10 1 1 0 0 1
refdes=R94
T 57400 45300 5 10 0 1 0 0 1
footprint=0603
}
N 57400 45400 57400 45700 4
C 57200 45700 1 0 0 3.3V-plus-1.sym
N 57900 45000 58300 45000 4
C 62500 42900 1 0 0 ksz8895-pmii.sym
{
T 62800 47900 5 10 1 1 0 0 1
value=KSZ8895
T 63800 47900 5 10 1 1 0 0 1
refdes=U2
}
T 62400 44000 9 10 1 0 90 0 1
Unused unless we take it to the FPGA.
T 62300 48400 9 10 1 0 0 0 1
INTR to CPU
N 54600 47700 54900 47700 4
N 52450 47000 54700 47000 4
N 54700 47000 54700 46500 4
N 54700 46500 54900 46500 4
T 59400 48800 9 20 1 0 0 0 1
10. Ethernet RMII
C 58100 46200 1 0 1 io-1.sym
{
T 57200 46400 5 10 0 0 0 6 1
net=SPIS:1
T 57900 46800 5 10 0 0 0 6 1
device=none
T 57800 46300 5 10 1 1 0 7 1
value=SPIS
}
C 58100 46700 1 0 1 io-1.sym
{
T 57200 46900 5 10 0 0 0 6 1
net=SPIC:1
T 57900 47300 5 10 0 0 0 6 1
device=none
T 57800 46800 5 10 1 1 0 7 1
value=SPIC
}
C 58100 46400 1 0 1 io-1.sym
{
T 57200 46600 5 10 0 0 0 6 1
net=SPID:1
T 57900 47000 5 10 0 0 0 6 1
device=none
T 57800 46500 5 10 1 1 0 7 1
value=SPID
}
C 58100 47100 1 0 1 io-1.sym
{
T 57200 47300 5 10 0 0 0 6 1
net=SPIQ:1
T 57900 47700 5 10 0 0 0 6 1
device=none
T 57800 47200 5 10 1 1 0 7 1
value=SPIQ
}
N 58100 46100 58100 46300 4
C 58100 43000 1 90 0 capacitor.sym
{
T 57400 43200 5 10 0 0 90 0 1
device=CAPACITOR
T 57500 43000 5 10 1 1 0 0 1
refdes=C78
T 57200 43200 5 10 0 0 90 0 1
symversion=0.1
T 58100 43000 5 10 0 1 0 0 1
footprint=0603
}
C 56100 43400 1 0 0 resistor-1.sym
{
T 56400 43800 5 10 0 0 0 0 1
device=RESISTOR
T 56400 43650 5 10 1 1 0 0 1
refdes=R24
T 56100 43400 5 10 0 1 0 0 1
footprint=0603
}
C 57000 43400 1 0 0 resistor-1.sym
{
T 57300 43800 5 10 0 0 0 0 1
device=RESISTOR
T 57300 43650 5 10 1 1 0 0 1
refdes=R31
T 57000 43400 5 10 0 1 0 0 1
footprint=0603
}
C 55900 43500 1 0 0 3.3V-plus-1.sym
C 57800 42700 1 0 0 gnd-1.sym
C 57000 43100 1 0 1 io-1.sym
{
T 56100 43300 5 10 0 0 0 6 1
net=SRST:1
T 56800 43700 5 10 0 0 0 6 1
device=none
T 56700 43200 5 10 1 1 0 7 1
value=SRST
}
N 57000 43200 57000 43500 4
N 57900 45000 57900 43500 4
N 55100 43200 55100 44000 4
N 55100 44000 57000 44000 4
N 55200 42900 55200 43900 4
N 55200 43900 57100 43900 4
C 61600 43700 1 0 0 resistor-1.sym
{
T 61900 44100 5 10 0 0 0 0 1
device=RESISTOR
T 61800 44000 5 10 1 1 0 0 1
refdes=R36
T 61600 43700 5 10 0 1 0 0 1
footprint=0603
}
C 61500 43500 1 0 0 gnd-1.sym
N 58100 47200 58100 48600 4
C 58100 48500 1 0 0 resistor-1.sym
{
T 58400 48900 5 10 0 0 0 0 1
device=RESISTOR
T 58300 48800 5 10 1 1 0 0 1
refdes=R58
T 58600 48500 5 10 0 1 0 0 1
footprint=0603
}
C 58800 48600 1 0 0 3.3V-plus-1.sym
C 55200 42800 1 0 0 resistor-1.sym
{
T 55500 43200 5 10 0 0 0 0 1
device=RESISTOR
T 55500 43050 5 10 1 1 0 0 1
refdes=R57
T 55500 42900 5 10 0 1 0 0 1
footprint=0603
}
N 56100 42900 56100 43500 4
C 52350 45300 1 0 0 gnd-1.sym
C 52650 45600 1 90 0 capacitor.sym
{
T 51950 45800 5 10 0 0 90 0 1
device=CAPACITOR
T 52200 45950 5 10 1 1 0 0 1
refdes=C82
T 51750 45800 5 10 0 0 90 0 1
symversion=0.1
}
C 52550 46100 1 90 0 resistor-1.sym
{
T 52150 46400 5 10 0 0 90 0 1
device=RESISTOR
T 52400 46850 5 10 1 1 90 0 1
refdes=R61
}
