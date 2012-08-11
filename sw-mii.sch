v 20110115 2
C 54900 44100 1 0 0 ksz8895-smii.sym
{
T 55200 49000 5 10 1 1 0 0 1
value=KSZ8895
T 56200 49000 5 10 1 1 0 0 1
refdes=U2
}
C 52600 42700 1 0 0 lpc-eth.sym
{
T 52700 49000 5 10 1 1 0 0 1
value=LPC ENET
T 53900 49000 5 10 1 1 0 0 1
refdes=U1
}
N 54900 47400 54500 47400 4
N 54900 45300 54500 45300 4
N 54900 45000 54500 45000 4
N 54900 46200 54500 46200 4
N 54500 46200 54500 46300 4
N 54900 48600 54500 48600 4
N 54500 48600 54500 48700 4
C 54600 43300 1 0 0 gnd-1.sym
N 54500 43600 54700 43600 4
C 58200 42700 1 0 0 ksz8895-io.sym
{
T 58600 48200 5 10 1 1 0 0 1
value=KSZ8895
T 59600 48200 5 10 1 1 0 0 1
refdes=U2
}
N 54500 43200 57000 43200 4
N 57000 43200 57000 47900 4
N 57000 47900 58300 47900 4
N 58300 47600 57100 47600 4
N 57100 47600 57100 42900 4
N 57100 42900 54500 42900 4
T 61100 43400 9 10 1 0 0 0 2
Float for
normal
T 61100 46700 9 10 1 0 0 0 2
Float for
normal
C 61000 47800 1 0 0 resistor-1.sym
{
T 61300 48200 5 10 0 0 0 0 1
device=RESISTOR
T 61200 48100 5 10 1 1 0 0 1
refdes=R?
T 61400 48100 5 10 1 1 0 0 1
value=DNP
}
C 61000 47500 1 0 0 resistor-1.sym
{
T 61300 47900 5 10 0 0 0 0 1
device=RESISTOR
T 61500 47300 5 10 1 1 0 0 1
refdes=R?
}
C 61700 47900 1 0 0 3.3V-plus-1.sym
N 61900 47900 61900 47600 4
N 61000 47200 61700 47200 4
N 58300 47200 57900 47200 4
N 58300 46800 57900 46800 4
N 58300 46500 57900 46500 4
N 58300 46100 57900 46100 4
C 57400 45600 1 0 0 resistor-1.sym
{
T 57700 46000 5 10 0 0 0 0 1
device=RESISTOR
T 57700 45900 5 10 1 1 0 0 1
refdes=R?
}
C 57400 45300 1 0 0 resistor-1.sym
{
T 57700 45700 5 10 0 0 0 0 1
device=RESISTOR
T 57500 45100 5 10 1 1 0 0 1
refdes=R?
T 57800 45100 5 10 1 1 0 0 1
value=DNP
}
N 57400 45400 57400 45700 4
C 57200 45700 1 0 0 3.3V-plus-1.sym
N 58300 45000 57700 45000 4
C 62500 42900 1 0 0 ksz8895-pmii.sym
{
T 62800 47900 5 10 1 1 0 0 1
value=KSZ8895
T 63800 47900 5 10 1 1 0 0 1
refdes=U2
}
T 62400 44000 9 10 1 0 90 0 1
Unused unless we take it to the FPGA.
T 57000 48400 9 10 1 0 0 0 5
INTR to CPU
RST_N to CPU
Serial to CPU
Pullups to correct power
(R)MII termination.
T 59500 48600 9 10 1 0 0 0 4
For RMII:
SMRXC
SMRXDV,SMRXD0/1
SMTXEN,SMTXD0/1
N 54500 47700 54900 47700 4
N 54500 47000 54700 47000 4
N 54700 47000 54700 46500 4
N 54700 46500 54900 46500 4
