#!/bin/bash

if [ "$1" = "stop" ]; then

  echo "Stopping Redis server instances"
  sudo killall redis-server

elif [ "$1" = "start" ]; then

  if [[ "$2" = "1" || "$2" = "2" ]]; then
  
    echo "Stopping Redis server instances"
    sudo killall redis-server
  
    echo "Starting Redis server instances"
  
    if [[ "$3" = "achal01" || "$3" = "achal02" || "$3" = "achal03" || "$3" = "achal04" ]]; then
      cluster_id="cluster_1"
    elif [[ "$3" = "achal05" || "$3" = "achal06" || "$3" = "achal07" || "$3" = "achal08" ]]; then
      cluster_id="cluster_2"
    else
      echo "Error: Incorrect third argument. Accepted: 'achal01'-'achal08'."
    fi
  
    if [[ "$3" = "achal01" || "$3" = "achal05" ]]; then 
      sudo taskset -c 0 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8080.conf
      sudo taskset -c 1 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8084.conf
      sudo taskset -c 2 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8088.conf
    elif [[ "$3" = "achal02" || "$3" = "achal06" ]]; then 
      sudo taskset -c 0 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8081.conf
      sudo taskset -c 1 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8085.conf
      sudo taskset -c 2 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8089.conf
    elif [[ "$3" = "achal03" || "$3" = "achal07" ]]; then 
      sudo taskset -c 0 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8082.conf
      sudo taskset -c 1 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8086.conf
      sudo taskset -c 2 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8090.conf
    elif [[ "$3" = "achal04" || "$3" = "achal08" ]]; then 
      sudo taskset -c 0 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8083.conf
      sudo taskset -c 1 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8087.conf
      sudo taskset -c 2 redis-server /home/pi/achal/config/redis/standalone/${cluster_id}/redis_8091.conf
    else
      echo "Error: Incorrect third argument. Accepted: 'achal01'-'achal08'."
    fi
  
  #elif [ "$2" = "2" ]; then
  #
  #  echo "Stopping Redis server instances"
  #  sudo killall redis-server
  #
  #  echo "Starting Redis server instances"
  #
  #  if [[ "$3" = "achal01" || "$3" = "achal05" ]]; then 
  #    sudo taskset -c 0 redis-server /home/pi/achal/config/redis/2/redis_8080.conf
  #    sudo taskset -c 1 redis-server /home/pi/achal/config/redis/2/redis_8084.conf
  #    sudo taskset -c 2 redis-server /home/pi/achal/config/redis/2/redis_8088.conf
  #  elif [[ "$3" = "achal02" || "$3" = "achal06" ]]; then 
  #    sudo taskset -c 0 redis-server /home/pi/achal/config/redis/2/redis_8081.conf
  #    sudo taskset -c 1 redis-server /home/pi/achal/config/redis/2/redis_8085.conf
  #    sudo taskset -c 2 redis-server /home/pi/achal/config/redis/2/redis_8089.conf
  #  elif [[ "$3" = "achal03" || "$3" = "achal07" ]]; then 
  #    sudo taskset -c 0 redis-server /home/pi/achal/config/redis/2/redis_8082.conf
  #    sudo taskset -c 1 redis-server /home/pi/achal/config/redis/2/redis_8086.conf
  #    sudo taskset -c 2 redis-server /home/pi/achal/config/redis/2/redis_8090.conf
  #  elif [[ "$3" = "achal04" || "$3" = "achal08" ]]; then 
  #    sudo taskset -c 0 redis-server /home/pi/achal/config/redis/2/redis_8083.conf
  #    sudo taskset -c 1 redis-server /home/pi/achal/config/redis/2/redis_8087.conf
  #    sudo taskset -c 2 redis-server /home/pi/achal/config/redis/2/redis_8091.conf
  #  else
  #    echo "Error: Incorrect third argument. Accepted: 'achal01'-'achal08'."
  #  fi

  else

    echo "Error: Incorrect second argument. Accepted: '1' or '2' (i.e., RedisKVS1 or RedisKVS2)."

  fi

else

  echo "Error: Incorrect first argument. Accepted: 'start' or 'stop'."

fi
