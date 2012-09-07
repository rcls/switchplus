v 20110115 2
C 39200 43100 1 0 0 lpc-emc.sym
{
T 39600 49600 5 10 1 1 0 0 1
value=LPC43 EMC
T 40700 48900 5 10 1 1 0 0 1
refdes=U1
}
C 43400 49700 1 180 1 sdram54-address.sym
{
T 43600 49700 5 10 1 1 180 6 1
description=54 BGA SDRAM
T 44100 46500 5 10 1 1 180 6 1
refdes=U4
T 44100 45900 5 10 1 1 180 6 1
footprint=vfbga54
}
C 36100 43500 1 0 0 sdram54-data.sym
{
T 36400 43500 5 10 1 1 0 0 1
description=54BGA SDRAM
T 36900 45500 5 10 1 1 0 0 1
refdes=U4
T 36800 44000 5 10 0 1 0 0 1
footprint=vfbga54
}
N 38200 46400 39200 46400 4
N 38400 45800 39200 45800 4
N 38500 45500 39200 45500 4
N 39200 44000 39200 43800 4
N 39200 43800 38000 43800 4
N 36100 46300 35900 46300 4
N 35900 46300 35900 47200 4
N 35900 47200 39200 47200 4
N 39200 47500 35800 47500 4
N 35800 47500 35800 46000 4
N 35800 46000 36100 46000 4
N 39200 47800 35700 47800 4
N 35700 47800 35700 45400 4
N 39200 48100 35600 48100 4
N 35600 48100 35600 45700 4
N 39200 48400 35500 48400 4
N 35500 48400 35500 44800 4
N 35500 44800 36100 44800 4
N 39200 48700 35400 48700 4
N 35400 48700 35400 45100 4
N 35400 45100 36100 45100 4
N 39200 49000 35300 49000 4
N 35300 49000 35300 44500 4
N 35300 44500 36100 44500 4
N 39200 49300 35200 49300 4
N 35200 49300 35200 44200 4
N 35200 44200 36100 44200 4
N 39200 46900 35100 46900 4
N 35100 46900 35100 43800 4
N 35100 43800 36100 43800 4
N 43400 46900 42700 46900 4
N 42700 46800 42300 46800 4
N 42700 46800 42700 46900 4
N 43400 46600 43100 46600 4
N 43100 50300 43100 46500 4
N 43100 46500 42300 46500 4
N 42300 46200 42800 46200 4
N 42800 46200 42800 46300 4
N 42800 46300 43400 46300 4
N 43400 46000 42800 46000 4
N 42800 46000 42800 45900 4
N 42800 45900 42300 45900 4
N 42300 45600 42800 45600 4
N 42800 45600 42800 45700 4
N 42800 45700 43400 45700 4
N 42300 49300 43400 49300 4
N 42300 49000 43400 49000 4
N 42300 48700 43400 48700 4
N 42300 48400 43400 48400 4
N 42300 48100 43400 48100 4
N 42300 47800 43400 47800 4
N 42300 47500 43400 47500 4
N 42300 47200 43400 47200 4
N 45100 49300 45300 49300 4
N 45300 49300 45300 45300 4
N 45300 45300 42300 45300 4
N 45100 49000 45400 49000 4
N 45400 49000 45400 45000 4
N 45400 45000 42300 45000 4
N 45100 48600 46100 48600 4
N 46100 48600 46100 43300 4
N 46100 43300 42300 43300 4
N 45100 48300 46000 48300 4
N 46000 48300 46000 43000 4
N 46000 43000 39200 43000 4
N 39200 43000 39200 43300 4
N 45100 48000 45900 48000 4
N 45900 48000 45900 43700 4
N 45900 43700 42300 43700 4
N 45100 47700 45800 47700 4
N 45800 47700 45800 44000 4
N 45800 44000 42300 44000 4
N 45100 47300 45700 47300 4
N 45700 47300 45700 44300 4
N 45700 44300 42300 44300 4
N 45100 46900 46500 46900 4
N 45600 46900 45600 44600 4
N 45600 44600 42300 44600 4
N 42900 49300 42900 50600 4
N 42800 46900 42800 50300 4
N 42700 47200 42700 50000 4
N 42600 47500 42600 49700 4
C 44000 50200 1 0 1 resistor-1.sym
{
T 43700 50600 5 10 0 0 0 6 1
device=RESISTOR
T 43700 50500 5 10 1 1 0 6 1
refdes=N4
T 44000 50200 5 10 0 1 0 0 1
footprint=0603
}
C 45100 50000 1 0 1 gnd-1.sym
C 42600 49600 1 0 1 resistor-1.sym
{
T 42300 50000 5 10 0 0 0 6 1
device=RESISTOR
T 41600 49600 5 10 1 1 0 6 1
refdes=N3
T 42600 49600 5 10 0 1 0 0 1
footprint=0603
}
C 42700 49900 1 0 1 resistor-1.sym
{
T 42400 50300 5 10 0 0 0 6 1
device=RESISTOR
T 41600 49900 5 10 1 1 0 6 1
refdes=R40
T 42700 49900 5 10 0 1 0 0 1
footprint=0603
}
C 42800 50200 1 0 1 resistor-1.sym
{
T 42500 50600 5 10 0 0 0 6 1
device=RESISTOR
T 41600 50200 5 10 1 1 0 6 1
refdes=N2
T 42800 50200 5 10 0 1 0 0 1
footprint=0603
}
C 42900 50500 1 0 1 resistor-1.sym
{
T 42600 50900 5 10 0 0 0 6 1
device=RESISTOR
T 41600 50500 5 10 1 1 0 6 1
refdes=R39
T 42900 50500 5 10 0 1 0 0 1
footprint=0603
}
N 42000 50600 41700 50600 4
N 41700 50600 41700 49700 4
N 41900 50300 41700 50300 4
N 41800 50000 41700 50000 4
N 41700 50100 41200 50100 4
C 41300 49800 1 0 1 gnd-1.sym
T 36700 49700 9 20 1 0 0 0 1
9. Memory
C 46700 45500 1 90 0 capacitor.sym
{
T 46000 45700 5 10 0 0 90 0 1
device=CAPACITOR
T 46400 46000 5 10 1 1 180 0 1
refdes=C48
T 45800 45700 5 10 0 0 90 0 1
symversion=0.1
T 46700 45500 5 10 0 1 0 0 1
footprint=0603
}
C 46600 46000 1 90 0 resistor-1.sym
{
T 46200 46300 5 10 0 0 90 0 1
device=RESISTOR
T 46400 46400 5 10 1 1 90 0 1
refdes=R41
T 46600 46000 5 10 0 1 0 0 1
footprint=0603
}
C 46400 45200 1 0 0 gnd-1.sym
N 35600 45700 36100 45700 4
N 35700 45400 36100 45400 4
N 38400 45800 38400 44200 4
N 38400 44200 38000 44200 4
N 38500 45500 38500 44800 4
N 38500 44800 38000 44800 4
N 38000 44500 38300 44500 4
N 38300 44500 38300 46100 4
N 38300 46100 39200 46100 4
N 38200 46400 38200 45400 4
N 38200 45400 38000 45400 4
N 39200 44300 38600 44300 4
N 38600 44300 38600 45100 4
N 38600 45100 38000 45100 4
N 39200 44600 38700 44600 4
N 38700 44600 38700 46000 4
N 38700 46000 38000 46000 4
N 38000 45700 38800 45700 4
N 38800 45700 38800 44900 4
N 38800 44900 39200 44900 4
N 38000 46300 38900 46300 4
N 38900 46300 38900 45200 4
N 38900 45200 39200 45200 4
C 44000 50300 1 0 0 switch-pushbutton-no-1.sym
{
T 44400 50100 5 10 1 1 0 0 1
refdes=S3
T 44400 50900 5 10 0 0 0 0 1
device=SWITCH_PUSHBUTTON_NO
T 44600 50200 5 10 0 1 0 0 1
footprint=tiny-tactile
}
