#!/bin/bash

if [ "$1" = "stop" ]; then

  echo "Stopping etcd server instances"
    sudo killall etcd

elif [ "$1" = "start" ]; then
    
    DATA_DIR="achal.etcd"

    MY_IP=$(ip addr show eth0 | grep "inet\b" | awk '{print $2}' | cut -d/ -f1)
    MY_HOSTNAME=$(hostname -s)

    rm -rf ${DATA_DIR}
    mkdir ${DATA_DIR}

    function gen_flag() {
        IP=$(nslookup $1 | grep 'Address\: ' | awk '{print $2}' | cut -d/ -f1)
        echo "${1}=http://${IP}:2380"
    }

    if [[ "$MY_HOSTNAME" = "achal01" || "$MY_HOSTNAME" = "achal02" || "$MY_HOSTNAME" = "achal03" || "$MY_HOSTNAME" = "achal04" ]]; then
      INITIAL_CLUSTER_TOKEN="etcd-cluster-1"
      INITIAL_CLUSTER="$(gen_flag achal01),$(gen_flag achal02),$(gen_flag achal03),$(gen_flag achal04)"
    elif [[ "$MY_HOSTNAME" = "achal05" || "$MY_HOSTNAME" = "achal06" || "$MY_HOSTNAME" = "achal07" || "$MY_HOSTNAME" = "achal08" ]]; then
      INITIAL_CLUSTER_TOKEN="etcd-cluster-2"
      INITIAL_CLUSTER="$(gen_flag achal05),$(gen_flag achal06),$(gen_flag achal07),$(gen_flag achal08)"
    else
      echo "Error: Incorrect hostname. Accepted: 'achal01'-'achal08'."
    fi

    ETCD_UNSUPPORTED_ARCH=arm etcd \
    --name ${MY_HOSTNAME} \
    --data-dir ${DATA_DIR} \
    --initial-advertise-peer-urls http://${MY_IP}:2380 \
    --listen-peer-urls http://${MY_IP}:2380 \
    --listen-client-urls http://${MY_IP}:2379,http://127.0.0.1:2379 \
    --advertise-client-urls http://${MY_IP}:2379 \
    --initial-cluster-token ${INITIAL_CLUSTER_TOKEN} \
    --initial-cluster ${INITIAL_CLUSTER} \
    --initial-cluster-state new \
    --heartbeat-interval 1000 \
    --election-timeout 5000

else

  echo "Error: Incorrect first argument. Accepted: 'start' or 'stop'."

fi
