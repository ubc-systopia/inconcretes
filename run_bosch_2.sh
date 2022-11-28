ts=`date "+%Y%m%d%H%M%S"`

# new config files with min period 100ms, under 1000 labels
#kzh15_100pcu_924l.cfg,  190, 
#kzh15_136pcu_1109l.cfg, 231,
#kzh15_114pcu_1930l.cfg, 395,
#kzh15_203pcu_1896l.cfg, 482,
#kzh15_126pcu_2506l.cfg, 572,
#kzh15_193pcu_2560l.cfg, 663,
#kzh15_102pcu_3196l.cfg, 774,
#kzh15_178pcu_3668l.cfg, 835,
#kzh15_200pcu_4099l.cfg, 979,
declare -a config_files2=( \
  "kzh15_100pcu_924l.cfg"  \
  "kzh15_136pcu_1109l.cfg" \
  "kzh15_114pcu_1930l.cfg" \
  "kzh15_203pcu_1896l.cfg" \
  "kzh15_126pcu_2506l.cfg" \
  "kzh15_193pcu_2560l.cfg" \
  "kzh15_102pcu_3196l.cfg" \
  "kzh15_178pcu_3668l.cfg" \
  "kzh15_200pcu_4099l.cfg")

for kvs in 'bft'; do
for cpus_used in 1; do
for period in 100; do
for config_file in "${config_files2[@]}"; do
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
