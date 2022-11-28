EXPDIR="paper/a1"
PLOTFILE="plot_a1.py"

DATADIR=`ls logging | tail -n 1`

# Bash uses $IFS variable to determine what the field separators are.
# By default $IFS is set to the space character.
# We overwrite it since our DATADIR is going to contain spaces 
IFS=$(echo -en "\n\b")

cp logging/${DATADIR}/*.data ${EXPDIR}/.
python3 ${EXPDIR}/${PLOTFILE}
