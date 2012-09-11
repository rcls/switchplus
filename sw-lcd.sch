v 20110115 2
C 52100 42400 1 0 0 spartan6-qfp144-bank3.sym
{
T 54000 45500 5 10 1 1 0 0 1
refdes=U3
T 52800 42500 5 10 1 1 0 0 1
device=Spartan6 Bank 3
T 54400 42500 5 10 1 1 0 0 1
footprint=qfp144
}
C 46400 42200 1 0 0 lpc-lcd.sym
{
T 48200 43100 5 10 1 1 0 0 1
value=LPC LCD
T 48600 44800 5 10 1 1 0 0 1
refdes=U1
}
N 55600 46900 55800 46900 4
N 49300 47000 50200 47000 4
N 49300 45500 50500 45500 4
N 49300 45200 50600 45200 4
N 49300 43800 50500 43800 4
N 46500 44800 44500 44800 4
N 49300 46100 50300 46100 4
N 49300 44400 50600 44400 4
N 46500 46400 45100 46400 4
N 45000 46700 46500 46700 4
N 46500 47300 44800 47300 4
N 44900 47000 46500 47000 4
N 49300 44100 51300 44100 4
N 46500 44500 45500 44500 4
N 46500 45500 45400 45500 4
N 44400 43600 46500 43600 4
T 56900 46000 9 20 1 0 0 0 2
8. FPGA Bank 3,
CPU LCD.
C 57000 41500 1 0 0 lpc-serial.sym
{
T 57400 44600 5 10 1 1 0 0 1
value=LPC Serial
T 58500 44600 5 10 1 1 0 0 1
refdes=U1
}
C 57000 42600 1 180 0 io-1.sym
{
T 56100 42400 5 10 0 0 180 0 1
net=SDA:1
T 56800 42000 5 10 0 0 180 0 1
device=none
T 56700 42500 5 10 1 1 180 1 1
value=SDA
}
C 57000 42900 1 180 0 io-1.sym
{
T 56100 42700 5 10 0 0 180 0 1
net=SCL:1
T 56800 42300 5 10 0 0 180 0 1
device=none
T 56700 42800 5 10 1 1 180 1 1
value=SCL
}
N 55600 44200 56500 44200 4
N 55600 43900 56600 43900 4
C 60200 44200 1 0 0 io-1.sym
{
T 61100 44400 5 10 0 0 0 0 1
net=SPIS:1
T 60400 44800 5 10 0 0 0 0 1
device=none
T 60500 44300 5 10 1 1 0 1 1
value=SPIS
}
C 60200 43900 1 0 0 io-1.sym
{
T 61100 44100 5 10 0 0 0 0 1
net=SPIC:1
T 60400 44500 5 10 0 0 0 0 1
device=none
T 60500 44000 5 10 1 1 0 1 1
value=SPIC
}
C 60200 43500 1 0 0 io-1.sym
{
T 61100 43700 5 10 0 0 0 0 1
net=SPID:1
T 60400 44100 5 10 0 0 0 0 1
device=none
T 60500 43600 5 10 1 1 0 1 1
value=SPID
}
C 60200 43200 1 0 0 io-1.sym
{
T 61100 43400 5 10 0 0 0 0 1
net=SPIQ:1
T 60400 43800 5 10 0 0 0 0 1
device=none
T 60500 43300 5 10 1 1 0 1 1
value=SPIQ
}
C 52100 41700 1 0 0 resistor-1.sym
{
T 52400 42100 5 10 0 0 0 0 1
device=RESISTOR
T 52000 41500 5 10 1 1 0 0 1
refdes=R38
T 52100 41700 5 10 0 1 0 0 1
footprint=0603
T 52100 41700 5 10 0 1 0 0 1
value=PTERM
}
C 53000 41600 1 0 0 capacitor.sym
{
T 53200 42300 5 10 0 0 0 0 1
device=CAPACITOR
T 52900 41400 5 10 1 1 0 0 1
refdes=C47
T 53200 42500 5 10 0 0 0 0 1
symversion=0.1
T 53000 41600 5 10 0 1 0 0 1
footprint=0603
T 53000 41600 5 10 0 1 0 0 1
value=PTERM
}
C 53400 41500 1 0 0 gnd-1.sym
N 51900 41800 52100 41800 4
N 57000 44000 56800 44000 4
N 56800 44000 56800 44500 4
N 56800 44500 55600 44500 4
N 50600 45200 50600 48100 4
N 50600 48100 56500 48100 4
N 56500 48100 56500 44200 4
N 44500 44800 44500 48200 4
N 44500 48200 56600 48200 4
N 56600 48200 56600 43900 4
N 55600 43600 57000 43600 4
N 50500 43800 50500 43100 4
N 50500 43100 52200 43100 4
N 50300 46100 50300 42800 4
N 57000 43300 56100 43300 4
N 56100 43300 56100 42200 4
N 56100 42200 52000 42200 4
N 52000 42200 52000 43400 4
N 52000 43400 52200 43400 4
N 50600 44400 50600 43700 4
N 50600 43700 52200 43700 4
N 45100 46400 45100 41300 4
N 45100 41300 51800 41300 4
N 51800 41300 51800 44300 4
N 51800 44300 52200 44300 4
N 44800 47300 44800 41400 4
N 44800 41400 51700 41400 4
N 51700 41400 51700 44900 4
N 51700 44900 52200 44900 4
N 44900 47000 44900 41500 4
N 44900 41500 51600 41500 4
N 51600 41500 51600 45200 4
N 51600 45200 52200 45200 4
N 51900 41800 51900 43700 4
N 45000 46700 45000 41600 4
N 45000 41600 51500 41600 4
N 51500 41600 51500 45500 4
N 51500 45500 52200 45500 4
N 49300 42400 51400 42400 4
N 51400 42400 51400 45800 4
N 51400 45800 52200 45800 4
N 51300 44100 51300 46100 4
N 51300 46100 52200 46100 4
N 45500 44500 45500 41700 4
N 45500 41700 51200 41700 4
N 51200 41700 51200 46400 4
N 51200 46400 52200 46400 4
N 45400 45500 45400 41800 4
N 45400 41800 51100 41800 4
N 51100 41800 51100 46700 4
N 51100 46700 52200 46700 4
C 46500 44100 1 0 1 io-1.sym
{
T 45600 44100 5 10 0 0 0 6 1
net=VD10:1
T 46300 44700 5 10 0 0 0 6 1
device=none
T 46200 44200 5 10 1 1 0 7 1
value=VD10
}
C 46500 43800 1 0 1 io-1.sym
{
T 45600 43800 5 10 0 0 0 6 1
net=VD11:1
T 46300 44400 5 10 0 0 0 6 1
device=none
T 46200 43900 5 10 1 1 0 7 1
value=VD11
}
N 46500 42700 44200 42700 4
N 44200 42700 44200 48000 4
N 44200 48000 56400 48000 4
N 56400 48000 56400 45100 4
N 56400 45100 55600 45100 4
N 44400 43600 44400 47900 4
N 44400 47900 56300 47900 4
N 56300 47900 56300 45400 4
N 56300 45400 55600 45400 4
N 46500 43000 44300 43000 4
N 44300 43000 44300 47800 4
N 44300 47800 56200 47800 4
N 56200 47800 56200 45700 4
N 56200 45700 55600 45700 4
N 50300 42800 52200 42800 4
N 57000 44300 57000 44800 4
N 57000 44800 55600 44800 4
N 49300 46700 50300 46700 4
N 50300 46700 50300 47700 4
N 50300 47700 56100 47700 4
N 56100 47700 56100 46000 4
N 56100 46000 55600 46000 4
N 49300 46400 50400 46400 4
N 50400 46400 50400 47600 4
N 50400 47600 56000 47600 4
N 56000 47600 56000 46300 4
N 56000 46300 55600 46300 4
N 50500 45500 50500 47500 4
N 50500 47500 55900 47500 4
N 55900 47500 55900 46600 4
N 55900 46600 55600 46600 4
N 50200 47000 50200 47400 4
N 50200 47400 55800 47400 4
N 55800 47400 55800 46900 4
C 49300 47200 1 0 0 io-1.sym
{
T 50200 47400 5 10 0 0 0 0 1
net=VD23:1
T 49500 47800 5 10 0 0 0 0 1
device=none
T 49600 47300 5 10 1 1 0 1 1
value=VD23
}
C 49300 45700 1 0 0 io-1.sym
{
T 50200 45900 5 10 0 0 0 0 1
net=VD18:1
T 49500 46300 5 10 0 0 0 0 1
device=none
T 49600 45800 5 10 1 1 0 1 1
value=VD18
}
C 49300 43400 1 0 0 io-1.sym
{
T 50200 43600 5 10 0 0 0 0 1
net=LCDLE:1
T 49500 44000 5 10 0 0 0 0 1
device=none
T 49500 43500 5 10 1 1 0 1 1
value=LCDLE
}
C 46500 45100 1 0 1 io-1.sym
{
T 45600 45300 5 10 0 0 0 6 1
net=VD7:1
T 46300 45700 5 10 0 0 0 6 1
device=none
T 46200 45200 5 10 1 1 0 7 1
value=VD7
}
C 46500 43200 1 0 1 io-1.sym
{
T 45600 43400 5 10 0 0 0 6 1
net=VD13:1
T 46300 43800 5 10 0 0 0 6 1
device=none
T 46200 43300 5 10 1 1 0 7 1
value=VD13
}
C 60200 42000 1 0 0 io-1.sym
{
T 61100 42200 5 10 0 0 0 0 1
net=UTX:1
T 60400 42600 5 10 0 0 0 0 1
device=none
T 60500 42100 5 10 1 1 0 1 1
value=UTX
}
C 60200 41700 1 0 0 io-1.sym
{
T 61100 41900 5 10 0 0 0 0 1
net=URX:1
T 60400 42300 5 10 0 0 0 0 1
device=none
T 60500 41800 5 10 1 1 0 1 1
value=URX
}
C 46500 46000 1 0 1 io-1.sym
{
T 45600 46200 5 10 0 0 0 6 1
net=VD4:1
T 46300 46600 5 10 0 0 0 6 1
device=none
T 46200 46100 5 10 1 1 0 7 1
value=VD4
}
C 46500 45700 1 0 1 io-1.sym
{
T 45600 45900 5 10 0 0 0 6 1
net=VD5:1
T 46300 46300 5 10 0 0 0 6 1
device=none
T 46200 45800 5 10 1 1 0 7 1
value=VD5
}
C 51900 47100 1 0 1 io-1.sym
{
T 51000 47300 5 10 0 0 0 6 1
net=G7.25:1
T 51700 47700 5 10 0 0 0 6 1
device=none
T 51600 47200 5 10 1 1 0 7 1
value=G7.25
}
C 51900 46900 1 0 1 io-1.sym
{
T 51000 47100 5 10 0 0 0 6 1
net=G7.24:1
T 51700 47500 5 10 0 0 0 6 1
device=none
T 51600 47000 5 10 1 1 0 7 1
value=G7.24
}
N 52100 47200 52100 44600 4
N 51900 47000 51900 44000 4
N 51900 44000 52200 44000 4
N 51900 47200 52100 47200 4
N 52100 44600 52200 44600 4
