v 20110115 2
C 39200 48400 1 0 0 spartan6-qfp144-misc.sym
{
T 41100 49200 5 10 1 1 0 0 1
refdes=U3
T 40000 48600 5 10 1 1 0 0 1
device=Spartan6 Misc
T 41300 48600 5 10 1 1 0 0 1
footprint=QFP144
}
C 44200 48100 1 0 1 connector6.sym
{
T 42400 49900 5 10 0 0 0 6 1
device=CONNECTOR_6
T 44100 50100 5 10 1 1 0 6 1
refdes=CONN1
T 44000 49700 5 10 0 1 0 0 1
footprint=6p6c
}
N 42700 49200 43400 49200 4
N 42700 48900 43400 48900 4
N 42700 49800 43400 49800 4
N 42700 49500 42900 49500 4
N 42900 49500 42900 48300 4
N 42900 48300 43400 48300 4
C 44300 49200 1 0 0 gnd-1.sym
C 44200 48600 1 0 0 3.3V-plus-1.sym
N 44400 48600 43400 48600 4
N 44400 49500 43400 49500 4
C 38200 50200 1 0 0 resistor-1.sym
{
T 38500 50600 5 10 0 0 0 0 1
device=RESISTOR
T 38500 50000 5 10 1 1 0 0 1
refdes=R11
T 38200 50200 5 10 0 1 0 0 1
footprint=0603
T 38600 50400 5 10 0 1 0 0 1
value=1k
}
C 37000 49700 1 0 0 resistor-1.sym
{
T 37300 50100 5 10 0 0 0 0 1
device=RESISTOR
T 37100 50000 5 10 1 1 0 0 1
refdes=R10
T 37000 49700 5 10 0 1 0 0 1
footprint=0603
T 37300 49800 5 10 0 1 0 0 1
value=100k
}
C 38200 50500 1 180 0 led-3.sym
{
T 37250 49850 5 10 0 0 180 0 1
device=LED
T 37950 50650 5 10 1 1 180 0 1
refdes=D1
T 37800 50300 5 10 0 1 90 0 1
footprint=s0805
}
C 38900 48600 1 0 0 gnd-1.sym
C 36900 49300 1 0 0 switch-pushbutton-no-1.sym
{
T 37300 49100 5 10 1 1 0 0 1
refdes=S2
T 37300 49900 5 10 0 0 0 0 1
device=SWITCH_PUSHBUTTON_NO
T 37500 49200 5 10 0 1 0 0 1
footprint=tiny-tactile
}
N 37900 49300 37900 49800 4
C 36800 49000 1 0 0 gnd-1.sym
C 39000 49300 1 90 0 capacitor.sym
{
T 38300 49500 5 10 0 0 90 0 1
device=CAPACITOR
T 38700 49400 5 10 1 1 180 0 1
refdes=C23
T 38100 49500 5 10 0 0 90 0 1
symversion=0.1
T 38800 49500 5 10 0 1 90 0 1
footprint=0603
T 38800 49600 5 10 0 1 90 0 1
value=470n
}
N 39100 50300 39100 49500 4
N 39100 49500 39300 49500 4
N 37000 50300 37000 49800 4
N 37300 50300 37000 50300 4
N 38800 49800 39300 49800 4
C 37900 49700 1 0 0 resistor-1.sym
{
T 38200 50100 5 10 0 0 0 0 1
device=RESISTOR
T 38100 49500 5 10 1 1 0 0 1
refdes=R12
T 38300 49800 5 10 0 1 0 0 1
footprint=0603
T 38300 49800 5 10 0 1 0 0 1
value=PULL
}
N 38800 49300 38800 48900 4
N 38800 48900 39300 48900 4
C 36800 50300 1 0 0 3.3V-plus-1.sym
C 38700 43100 1 0 0 spartan6-qfp144-bank1.sym
{
T 41000 46300 5 10 1 1 0 0 1
refdes=U3
T 39300 43300 5 10 1 1 0 0 1
device=Spartan6 QFP144 Bank 1
T 41500 43300 5 10 1 1 0 0 1
footprint=QFP144
}
C 37000 47300 1 0 0 resistor-1.sym
{
T 37300 47700 5 10 0 0 0 0 1
device=RESISTOR
T 37400 47600 5 10 1 1 0 0 1
refdes=R13
T 37000 47300 5 10 0 1 0 0 1
footprint=0603
}
C 37000 46700 1 0 0 resistor-1.sym
{
T 37300 47100 5 10 0 0 0 0 1
device=RESISTOR
T 37200 47000 5 10 1 1 0 0 1
refdes=R14
T 37000 46700 5 10 0 1 0 0 1
footprint=0603
}
C 37000 46100 1 0 0 resistor-1.sym
{
T 37300 46500 5 10 0 0 0 0 1
device=RESISTOR
T 37200 46400 5 10 1 1 0 0 1
refdes=R15
T 37000 46100 5 10 0 1 0 0 1
footprint=0603
}
C 37000 45200 1 0 0 resistor-1.sym
{
T 37300 45600 5 10 0 0 0 0 1
device=RESISTOR
T 37200 45500 5 10 1 1 0 0 1
refdes=R16
T 37000 45200 5 10 0 1 0 0 1
footprint=0603
}
C 43500 47100 1 0 0 resistor-1.sym
{
T 43800 47500 5 10 0 0 0 0 1
device=RESISTOR
T 43700 47400 5 10 1 1 0 0 1
refdes=R17
T 43500 47100 5 10 0 1 0 0 1
footprint=0603
}
C 43500 46500 1 0 0 resistor-1.sym
{
T 43800 46900 5 10 0 0 0 0 1
device=RESISTOR
T 43700 46800 5 10 1 1 0 0 1
refdes=R18
T 43500 46500 5 10 0 1 0 0 1
footprint=0603
}
C 43500 45600 1 0 0 resistor-1.sym
{
T 43800 46000 5 10 0 0 0 0 1
device=RESISTOR
T 43700 45900 5 10 1 1 0 0 1
refdes=R19
T 43500 45600 5 10 0 1 0 0 1
footprint=0603
}
C 43500 45000 1 0 0 resistor-1.sym
{
T 43800 45400 5 10 0 0 0 0 1
device=RESISTOR
T 43700 45300 5 10 1 1 0 0 1
refdes=R20
T 43500 45000 5 10 0 1 0 0 1
footprint=0603
}
C 42600 47000 1 0 0 led-3.sym
{
T 43550 47650 5 10 0 0 0 0 1
device=LED
T 43250 47350 5 10 1 1 0 0 1
refdes=D6
T 42600 47000 5 10 0 1 0 0 1
footprint=s0805
}
C 42600 46400 1 0 0 led-3.sym
{
T 43550 47050 5 10 0 0 0 0 1
device=LED
T 43250 46750 5 10 1 1 0 0 1
refdes=D7
T 42600 46400 5 10 0 1 0 0 1
footprint=s0805
}
C 42600 45500 1 0 0 led-3.sym
{
T 43550 46150 5 10 0 0 0 0 1
device=LED
T 43250 45850 5 10 1 1 0 0 1
refdes=D8
T 42600 45500 5 10 0 1 0 0 1
footprint=s0805
}
C 42600 44900 1 0 0 led-3.sym
{
T 43550 45550 5 10 0 0 0 0 1
device=LED
T 43250 45250 5 10 1 1 0 0 1
refdes=D9
T 42600 44900 5 10 0 1 0 0 1
footprint=s0805
}
C 38800 45100 1 0 1 led-3.sym
{
T 37850 45750 5 10 0 0 0 6 1
device=LED
T 38150 45450 5 10 1 1 0 6 1
refdes=D5
T 38800 45100 5 10 0 1 0 0 1
footprint=s0805
}
C 38800 46000 1 0 1 led-3.sym
{
T 37850 46650 5 10 0 0 0 6 1
device=LED
T 38150 46350 5 10 1 1 0 6 1
refdes=D4
T 38800 46000 5 10 0 1 0 0 1
footprint=s0805
}
C 38800 46600 1 0 1 led-3.sym
{
T 37850 47250 5 10 0 0 0 6 1
device=LED
T 38150 46950 5 10 1 1 0 6 1
refdes=D3
T 38800 46600 5 10 0 1 0 0 1
footprint=s0805
}
C 38800 47200 1 0 1 led-3.sym
{
T 37850 47850 5 10 0 0 0 6 1
device=LED
T 38150 47550 5 10 1 1 0 6 1
refdes=D2
T 38800 47200 5 10 0 1 0 0 1
footprint=s0805
}
N 37000 45300 37000 47400 4
N 44400 45000 44400 47200 4
C 44200 47200 1 0 0 3.3V-plus-1.sym
C 36800 47400 1 0 0 3.3V-plus-1.sym
C 45200 43500 1 0 1 txo-1.sym
{
T 45000 44400 5 10 1 1 0 6 1
refdes=U61
T 45000 45500 5 10 0 0 0 6 1
device=VTXO
T 44300 44500 5 10 0 1 0 0 1
footprint=smt-osc-3mm.fp
}
N 42600 44200 43600 44200 4
N 45200 44200 45200 43900 4
N 45200 43900 42600 43900 4
N 44400 45000 45300 45000 4
C 45500 44500 1 90 0 capacitor.sym
{
T 44800 44700 5 10 0 0 90 0 1
device=CAPACITOR
T 45500 44900 5 10 1 1 90 0 1
refdes=C24
T 44600 44700 5 10 0 0 90 0 1
symversion=0.1
T 45500 44500 5 10 0 1 0 0 1
footprint=0603
}
N 44400 43500 45300 43500 4
N 45300 43500 45300 44500 4
C 44800 43200 1 0 0 gnd-1.sym
T 39000 47900 9 20 1 0 0 0 1
4. FPGA Bank 1 and Misc.
