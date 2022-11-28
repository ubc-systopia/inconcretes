ts=`date "+%Y%m%d%H%M%S"`

for kvs in 'bft'; do
for cpus_used in 1; do
for tasks_per_cpu in 4; do
for period in 50; do
for fault_mode in 0 1 2; do
for num_faulty in 1 2; do

#if [[ $tasks_per_cpu = 1 ]]; then
#  ivp_task_wcet_ms=1
#else
#  ivp_task_wcet_ms=1
#fi

kvs_task_wcet_ms=$(echo "${period} - ${tasks_per_cpu} * 1" | bc)

cmd="python3 run.py"
cmd+=" --cid 1"
cmd+=" --exp ivp"
cmd+=" --duration 100"
cmd+=" --kvs ${kvs}"
cmd+=" --cpus_used ${cpus_used}"
cmd+=" --tasks_per_cpu ${tasks_per_cpu}"
cmd+=" --period ${period}"
cmd+=" --ivp_task_wcet_ms 1"
cmd+=" --kvs_task_wcet_ms ${kvs_task_wcet_ms}"
cmd+=" --output_file ${ts}.data"
cmd+=" --fault_mode ${fault_mode}"
cmd+=" --num_faulty ${num_faulty}"

echo "Command: $cmd"

eval "$cmd"

done
done
done
done
done
done
