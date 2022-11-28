# Output in PNG image file
set term png
set output "output.png"
set xtics font "Verdana,10"
set ytics font "Verdana,6"
# for multiple plot in one image
set multiplot
set size 1, 0.3
set origin 0.0,0.6
plot 'ptp.dat' using 1:2 with lines title 'offset' lc 'black'
set origin 0.0,0.3
plot 'ptp.dat' using 1:3 with lines title 'freq' lc 'black'
set origin 0.0,0.0
plot 'ptp.dat' using 1:4 with lines title 'pathDelay' lc 'black'