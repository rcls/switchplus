v 20110115 2
C 41000 42300 1 0 0 lpc-misc.sym
{
T 41400 49000 5 10 1 1 0 0 1
value=LPC Misc
T 42300 49000 5 10 1 1 0 0 1
refdes=U1
T 42600 44400 5 10 1 1 0 0 1
footprint=lbga256
}
C 33500 46300 1 0 0 lpc-gpio.sym
{
T 33900 50300 5 10 1 1 0 0 1
value=LPC GPIO
T 34900 50300 5 10 1 1 0 0 1
refdes=U1
}
C 33400 41500 1 0 0 lpc-gpio6.sym
{
T 33700 45900 5 10 1 1 0 0 1
value=LPC GPIO6
T 35000 45900 5 10 1 1 0 0 1
refdes=U1
}
C 37400 41500 1 0 0 lpc-gpio7.sym
{
T 37800 45000 5 10 1 1 0 0 1
value=LPC GPIO7
T 38900 45000 5 10 1 1 0 0 1
refdes=U1
}
T 40300 42000 9 10 1 0 0 0 1
Hook-up unused pins as convenient...
C 39800 45400 1 90 0 capacitor.sym
{
T 39100 45600 5 10 0 0 90 0 1
device=CAPACITOR
T 39350 45700 5 10 1 1 180 0 1
refdes=C2
T 38900 45600 5 10 0 0 90 0 1
symversion=0.1
T 39800 45400 5 10 0 1 0 0 1
footprint=0603
}
C 40900 45400 1 90 0 capacitor.sym
{
T 40200 45600 5 10 0 0 90 0 1
device=CAPACITOR
T 40850 45300 5 10 1 1 180 0 1
refdes=C3
T 40000 45600 5 10 0 0 90 0 1
symversion=0.1
T 40900 45400 5 10 0 1 0 0 1
footprint=0603
}
N 40500 45900 41000 45900 4
N 39600 45900 39600 46200 4
N 39600 46200 41000 46200 4
N 39600 45400 40700 45400 4
C 40000 45100 1 0 0 gnd-1.sym
C 39700 46800 1 0 0 resistor-1.sym
{
T 40000 47200 5 10 0 0 0 0 1
device=RESISTOR
T 40200 47100 5 10 1 1 0 0 1
refdes=R2
T 39700 46800 5 10 0 1 0 0 1
footprint=0603
}
C 38800 46800 1 0 0 resistor-1.sym
{
T 39100 47200 5 10 0 0 0 0 1
device=RESISTOR
T 39200 47100 5 10 1 1 0 0 1
refdes=R1
T 38800 46800 5 10 0 1 0 0 1
footprint=0603
}
C 40800 46400 1 90 0 capacitor.sym
{
T 40100 46600 5 10 0 0 90 0 1
device=CAPACITOR
T 40350 46700 5 10 1 1 180 0 1
refdes=C1
T 39900 46600 5 10 0 0 90 0 1
symversion=0.1
T 40800 46400 5 10 0 1 0 0 1
footprint=0603
}
N 40600 46900 41000 46900 4
N 41000 46900 41000 46700 4
C 38700 46500 1 0 0 switch-pushbutton-no-1.sym
{
T 38500 46500 5 10 1 1 0 0 1
refdes=S1
T 39100 47100 5 10 0 0 0 0 1
device=SWITCH_PUSHBUTTON_NO
T 39200 46600 5 10 0 1 0 0 1
footprint=tiny-tactile
}
N 39700 46500 39700 46900 4
N 40600 46400 38700 46400 4
N 38700 46400 38700 46500 4
C 38600 46100 1 0 0 gnd-1.sym
C 38600 46900 1 0 0 3.3V-plus-1.sym
T 37000 48200 9 20 1 0 0 0 1
1. CPU Misc & Extras
C 39800 45500 1 0 0 xtal-quad.sym
{
T 40000 46200 5 10 1 1 0 0 1
refdes=X1
T 40200 45900 5 10 0 1 0 0 1
footprint=smt-xtal-3mm
}
N 39600 45900 39800 45900 4
N 40000 45500 40000 45400 4
N 40300 45500 40300 45400 4
C 37400 41900 1 0 1 io-1.sym
{
T 36500 42100 5 10 0 0 0 6 1
net=SRST:1
T 37200 42500 5 10 0 0 0 6 1
device=none
T 37100 42000 5 10 1 1 0 7 1
value=SRST
}
