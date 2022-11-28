#!/bin/bash

SCRIPTS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD="${SCRIPTS}/../build/"
TRACEFILE="${SCRIPTS}/ivp_sim.csv"
PLOTFILE="${SCRIPTS}/ivp_sim.pdf"

rm -f ${TRACEFILE} ${PLOTFILE}

#USAGE:
#  ivp_sim DT KP KI KD MAX_COUNT INJECT_FAULTS TRACE_FILE REAL_TIME
#DESCRIPTION:
#  Run an inverted pendulum simulation, where
#  DT (float) denotes the time period in seconds;
#  KP, KI, KD (floats) denote the PID constants, respectively;
#  MAX_COUNT (int) denotes the number of simulation iterations;
#  INJECT_FAULTS (0 or 1) denotes whether faults are injected;
#  TRACE_FILE (string) denotes where the trace output is flushed; and
#  REAL_TIME (0 or 1) denotes if simulation is discrete or real-time
${BUILD}/ivp_sim 0.05 1000 100 100 1000 0 ${TRACEFILE} 1

python3 ${SCRIPTS}/ivp_sim_plot.py -i ${TRACEFILE} -o ${PLOTFILE}
#open ${PLOTFILE}
