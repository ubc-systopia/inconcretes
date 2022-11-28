ts=`date "+%Y%m%d%H%M%S"`

# configs with min period 50 ms amd under 1000 labels
#kzh15_113pcu_1018l.cfg 270, 
#kzh15_109pcu_1479l.cfg 363, 
#kzh15_108pcu_1557l.cfg 440, 
#kzh15_140pcu_2017l.cfg 573, 
#kzh15_125pcu_2541l.cfg 689, 
#kzh15_103pcu_2846l.cfg 744, 
#kzh15_105pcu_2965l.cfg 849, 
#kzh15_118pcu_3461l.cfg 905, 
declare -a config_files0=( \
  "kzh15_113pcu_1018l.cfg" \
  "kzh15_109pcu_1479l.cfg" \
  "kzh15_94pcu_1891l.cfg" \
  "kzh15_140pcu_2017l.cfg" \
  "kzh15_125pcu_2541l.cfg" \
  "kzh15_103pcu_2846l.cfg" \
  "kzh15_105pcu_2965l.cfg" \
  "kzh15_118pcu_3461l.cfg")

for kvs in 'bft'; do
for cpus_used in 1; do
for period in 50; do
for config_file in "${config_files0[@]}"; do
for num_trials in `seq 1 1`; do

#if [[ $tasks_per_cpu = 1 ]]; then
#  ivp_task_wcet_ms=1
#else
#  ivp_task_wcet_ms=1
#fi

#if [[ $fault_mode = 0 ]]; then
#  num_faulty=0
#fi

#kvs_task_wcet_ms=$(echo "${period} - ${tasks_per_cpu} * 1" | bc)

cmd="python3 run.py"
cmd+=" --cid 1"
cmd+=" --exp bosch"
cmd+=" --duration 300"
cmd+=" --kvs ${kvs}"
cmd+=" --cpus_used ${cpus_used}"
#cmd+=" --tasks_per_cpu ${tasks_per_cpu}"
cmd+=" --period ${period}"
#cmd+=" --ivp_task_wcet_ms 1"
#cmd+=" --kvs_task_wcet_ms ${kvs_task_wcet_ms}"
cmd+=" --output_file ${ts}.data"
cmd+=" --config_file ${config_file}"
#cmd+=" --fault_mode ${fault_mode}"
#cmd+=" --num_faulty ${num_faulty}"

echo "$num_trials Command: $cmd"

eval "$cmd"

done
done
done
done
done
