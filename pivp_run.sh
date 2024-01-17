#!/bin/bash

# List of Raspberry Pi hostnames or IP addresses
PI_ADDRESS="achal"

# C++ program to execute on each Raspberry Pi
CXX_PROGRAM="build/run_pivp"

# Username for SSH access
USERNAME="pi"

# Parameters for the C++ program

NUM_NODES="$1"
POS_PERIOD="$2"
ANG_PERIOD="$3"

KP_POS="$4"  
KD_POS="$5"  
KI_POS="$6"   

KP_ANG="$7"  
KD_ANG="$8"  
KI_ANG="$9"

POS_SET="${10}"
ANG_SET="${11}"

# Directory where the C++ program is located on each Raspberry Pi
REMOTE_DIR="~/IVP_System"

# Loop through the Raspberry Pis and execute the program
for ((NODE_NUM=2; NODE_NUM<=(NUM_NODES); NODE_NUM++)); do
    # Pad the node number with leading zeros if necessary
    PI="${PI_ADDRESS}$(printf "%02d" "$NODE_NUM")"
    
    echo "Executing program on $PI"

    # Use SSH to execute the C++ program on the Raspberry Pi
    ssh -f -i /home/pi/.ssh/id_rsa_ubc "pi@$PI" "\
        cd $REMOTE_DIR && sudo \
        ./$CXX_PROGRAM $NODE_NUM \
        $NUM_NODES $POS_PERIOD \
        $ANG_PERIOD $KP_POS $KD_POS \
        $KI_POS $KP_ANG $KD_ANG $KI_ANG \
        $POS_SET $ANG_SET \
        > /dev/null 2>&1"
   
    # Close the SSH session immediately after starting the program
    # ssh "$USERNAME@$PI" "exit"
done


# Wait for all SSH sessions to finish
echo "All programs executed"
sudo  ./$CXX_PROGRAM 1 \
        $NUM_NODES $POS_PERIOD \
        $ANG_PERIOD $KP_POS $KD_POS \
        $KI_POS $KP_ANG $KD_ANG $KI_ANG \
        $POS_SET $ANG_SET \
