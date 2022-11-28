# Parser script to plot PTP delay
# Vincent Jordan
# 2020.10.12
# Run with:
# journalctl -u ptp4l.service | grep "master offset" | python3 parse_ptp.py > ptp.dat
# gnuplot -c plot_ptp.gp
import re
import fileinput
import sys
minKernelTime = 0;
maxKernelTime = 1000;
pattern = '^(.+)ptp4l\[[0-9]+\]: \[(.+)\] master offset\s+(-?[0-9]+) s([012]) freq\s+([+-]\d+) path delay\s+(-?\d+)$'
test_string = 'Oct 16 13:49:00 achal04 ptp4l[3406]: [192966.306] master offset   -5123868 s0 freq   +6713 path delay     71707'

# Gnuplot data header
print('# time, offset, freq, pathDelay')
for line in fileinput.input():
    # Regex search
    res = re.search(pattern, line)
    # if pattern was matched
    if res:
        # Capture result
        timeAndHost  = res.group(1)
        kernelTime   = res.group(2)
        masterOffset = res.group(3)
        state        = res.group(4)
        freq         = res.group(5)
        pathDelay    = res.group(6)

        # if (state == '2') and (float(kernelTime) > minKernelTime) and (float(kernelTime) < maxKernelTime):
        print(kernelTime, masterOffset, freq, pathDelay)
        
    # if issue in patter
    else:
        print("Regex error:", line)
        sys.exit()