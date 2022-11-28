ts=`date "+%Y%m%d%H%M%S"`

# new config files with min period 100ms, above 1000 labels
#kzh15_234pcu_3982l.cfg, 1001
#kzh15_272pcu_4606l.cfg, 1157
#kzh15_472pcu_4901l.cfg, 1218
#kzh15_347pcu_5428l.cfg, 1320
#kzh15_189pcu_5666l.cfg, 1420
#kzh15_273pcu_6333l.cfg, 1531
#kzh15_184pcu_6771l.cfg, 1682
#kzh15_269pcu_7004l.cfg, 1712
#kzh15_287pcu_7461l.cfg, 1810
#kzh15_166pcu_8004l.cfg, 1986
declare -a config_files3=( \
  "kzh15_234pcu_3982l.cfg" \
  "kzh15_273pcu_6333l.cfg" \
  "kzh15_166pcu_8004l.cfg")

for kvs in 'bft'; do
for cpus_used in 1; do
for period in 100; do
for config_file in "${config_files3[@]}"; do
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
