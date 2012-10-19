v 20110115 2
C 41000 44800 1 0 0 hdmi.sym
{
T 41900 50600 5 10 0 1 0 0 1
footprint=molex-hdmi
T 41000 50600 5 10 1 1 0 0 1
refdes=CONN2
T 41100 49100 5 10 1 1 0 0 1
value=HDMI
}
N 44900 47200 44900 45500 4
N 44900 45500 42300 45500 4
N 44700 45800 44700 47500 4
N 44700 45800 42300 45800 4
N 44600 46400 44600 47800 4
N 44600 46400 42300 46400 4
N 44400 46700 44400 49000 4
N 44400 46700 42300 46700 4
N 44300 47300 44300 49100 4
N 44300 47300 42300 47300 4
N 44100 47600 44100 49300 4
N 44100 47600 42300 47600 4
N 42300 48200 44000 48200 4
N 42300 50300 43600 50300 4
N 43600 50300 43600 49700 4
N 43600 49900 50300 49900 4
N 42300 48800 43700 48800 4
N 43700 48800 43700 49800 4
N 43700 49800 50200 49800 4
N 42300 49700 42700 49700 4
N 42400 49700 42400 45200 4
N 42300 45200 42400 45200 4
N 42300 46100 42400 46100 4
N 42300 47000 42400 47000 4
N 42300 47900 42400 47900 4
C 42700 49600 1 0 0 resistor-1.sym
{
T 43000 50000 5 10 0 0 0 0 1
device=RESISTOR
T 42800 49900 5 10 1 1 0 0 1
refdes=R24
T 43200 49900 5 10 1 1 0 0 1
value=1k
T 43000 49700 5 10 0 1 0 0 1
footprint=0603
}
T 45200 44400 9 10 1 0 0 0 1
Float
N 42300 48500 43900 48500 4
C 42600 45000 1 0 0 gnd-1.sym
N 42700 45300 42400 45300 4
C 42700 49300 1 0 0 io-1.sym
{
T 43600 49500 5 10 0 0 0 0 1
net=SDA:1
T 42900 49900 5 10 0 0 0 0 1
device=none
T 43000 49400 5 10 1 1 0 1 1
value=SDA
}
C 42700 49000 1 0 0 io-1.sym
{
T 43600 49200 5 10 0 0 0 0 1
net=SCL:1
T 42900 49600 5 10 0 0 0 0 1
device=none
T 43000 49100 5 10 1 1 0 1 1
value=SCL
}
T 45600 50200 9 20 1 0 0 0 1
6. FPGA Bank 0, HDMI
N 42300 49100 42700 49100 4
N 42700 49400 42300 49400 4
C 42900 50600 1 0 0 resistor-1.sym
{
T 43200 51000 5 10 0 0 0 0 1
device=RESISTOR
T 42700 50750 5 10 1 1 0 0 1
refdes=R23
T 42900 50600 5 10 0 1 0 0 1
footprint=0603
T 42900 50600 5 10 0 1 0 0 1
value=100k
}
C 42900 50400 1 0 0 resistor-1.sym
{
T 43200 50800 5 10 0 0 0 0 1
device=RESISTOR
T 43650 50300 5 10 1 1 0 0 1
refdes=R25
T 42900 50400 5 10 0 1 0 0 1
footprint=0603
T 42900 50400 5 10 0 1 0 0 1
value=100k
}
N 42500 49100 42500 50700 4
N 42500 50700 42900 50700 4
N 42600 49400 42600 50500 4
N 42600 50500 42900 50500 4
N 42300 50000 42400 50000 4
N 42400 50000 42400 51000 4
N 42400 51000 43800 51000 4
N 43800 51000 43800 50500 4
C 44600 50500 1 0 0 5V-plus-1.sym
C 44800 50400 1 90 0 jumper-1.sym
{
T 44300 50700 5 8 0 0 90 0 1
device=JUMPER
T 44200 50650 5 10 1 1 0 0 1
refdes=J4
T 44200 50500 5 10 0 1 0 0 1
footprint=sip2
}
C 49500 48800 1 180 0 spartan6-qfp144-bank0.sym
{
T 48700 48700 5 10 1 1 180 0 1
device=Spartan6 Bank 0
T 47200 48700 5 10 1 1 180 0 1
footprint=qfp144
T 47400 45500 5 10 1 1 180 0 1
refdes=U3
}
N 44900 47200 45900 47200 4
N 42300 44900 43100 44900 4
N 45000 45300 45000 46900 4
N 45000 46900 45900 46900 4
N 43900 48500 43900 49600 4
N 43900 49600 50000 49600 4
N 50000 49600 50000 47200 4
N 50200 49800 50200 46600 4
N 50200 46600 49300 46600 4
N 50300 49900 50300 46300 4
N 50300 46300 49300 46300 4
N 50000 47200 49300 47200 4
N 44000 48200 44000 49400 4
N 44000 49400 49800 49400 4
N 44100 49300 49700 49300 4
N 49800 49400 49800 47500 4
N 49800 47500 49300 47500 4
N 49700 49300 49700 47800 4
N 49700 47800 49300 47800 4
N 44300 49100 49500 49100 4
N 44400 49000 49400 49000 4
N 49400 49000 49400 48400 4
N 49400 48400 49300 48400 4
N 49500 49100 49500 48100 4
N 49500 48100 49300 48100 4
N 44600 47800 45900 47800 4
N 44700 47500 45900 47500 4
C 45900 44900 1 180 0 io-1.sym
{
T 45000 44900 5 10 0 0 180 0 1
net=VD10:1
T 45700 44300 5 10 0 0 180 0 1
device=none
T 45600 44800 5 10 1 1 180 1 1
value=VD10
}
C 45900 45200 1 180 0 io-1.sym
{
T 45000 45200 5 10 0 0 180 0 1
net=VD11:1
T 45700 44600 5 10 0 0 180 0 1
device=none
T 45600 45100 5 10 1 1 180 1 1
value=VD11
}
N 43100 44900 43100 45300 4
N 43100 45300 45000 45300 4
C 45900 45600 1 0 1 io-1.sym
{
T 45000 45800 5 10 0 0 0 6 1
net=VD4:1
T 45700 46200 5 10 0 0 0 6 1
device=none
T 45600 45700 5 10 1 1 0 7 1
value=VD4
}
C 45900 45300 1 0 1 io-1.sym
{
T 45000 45500 5 10 0 0 0 6 1
net=VD5:1
T 45700 45900 5 10 0 0 0 6 1
device=none
T 45600 45400 5 10 1 1 0 7 1
value=VD5
}