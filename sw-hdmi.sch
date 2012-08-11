v 20110115 2
C 44000 45700 1 0 0 spartan6-qfp144-bank0.sym
{
T 44800 45800 5 10 1 1 0 0 1
device=Spartan6 Bank 0
T 46300 45800 5 10 1 1 0 0 1
footprint=QFP144
T 46100 49000 5 10 1 1 0 0 1
refdes=U?
}
C 41000 44700 1 0 0 hdmi.sym
{
T 41900 50500 5 10 0 1 0 0 1
footprint=molex-hdmi
T 41000 50500 5 10 1 1 0 0 1
refdes=CONN?
T 41100 49000 5 10 1 1 0 0 1
value=HDMI
}
N 44200 46100 43700 46100 4
N 43700 46100 43700 44800 4
N 43700 44800 42300 44800 4
N 44200 46400 43600 46400 4
N 43600 46400 43600 45400 4
N 43600 45400 42300 45400 4
N 44200 46700 43400 46700 4
N 43400 46700 43400 45700 4
N 43400 45700 42300 45700 4
N 44200 47000 43300 47000 4
N 43300 47000 43300 46300 4
N 43300 46300 42300 46300 4
N 44200 47300 43100 47300 4
N 43100 47300 43100 46600 4
N 43100 46600 42300 46600 4
N 44200 47600 43000 47600 4
N 43000 47600 43000 47200 4
N 43000 47200 42300 47200 4
N 44200 47900 42800 47900 4
N 42800 47900 42800 47500 4
N 42800 47500 42300 47500 4
N 44200 48200 42700 48200 4
N 42700 48200 42700 48100 4
N 42700 48100 42300 48100 4
N 42300 50200 43600 50200 4
N 43600 50200 43600 49400 4
N 43600 49400 44200 49400 4
N 42300 48700 43600 48700 4
N 43600 48700 43600 49100 4
N 43600 49100 44200 49100 4
N 42300 49600 42700 49600 4
N 42400 49600 42400 45100 4
N 42300 45100 42400 45100 4
N 42300 46000 42400 46000 4
N 42300 46900 42400 46900 4
N 42300 47800 42400 47800 4
C 42700 49500 1 0 0 resistor-1.sym
{
T 43000 49900 5 10 0 0 0 0 1
device=RESISTOR
T 42900 49800 5 10 1 1 0 0 1
refdes=R?
T 43200 49800 5 10 1 1 0 0 1
value=1k
}
T 47900 50000 9 10 1 0 0 0 1
Float
N 42300 48400 43800 48400 4
N 43800 48400 43800 48500 4
N 43800 48500 44200 48500 4
T 44400 45200 9 10 1 0 0 0 1
TODO: Maybe put oscillator on this bank.
C 42600 44900 1 0 0 gnd-1.sym
N 42700 45200 42400 45200 4
C 42300 49200 1 0 0 io-1.sym
{
T 43200 49400 5 10 0 0 0 0 1
net=SDA:1
T 42500 49800 5 10 0 0 0 0 1
device=none
T 42600 49300 5 10 1 1 0 1 1
value=SDA
}
C 42300 48900 1 0 0 io-1.sym
{
T 43200 49100 5 10 0 0 0 0 1
net=SCL:1
T 42500 49500 5 10 0 0 0 0 1
device=none
T 42600 49000 5 10 1 1 0 1 1
value=SCL
}
