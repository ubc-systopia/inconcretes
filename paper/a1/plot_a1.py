import os
import sys
import glob
import numpy as np
import matplotlib.pyplot as plt

dirname = os.path.dirname(os.path.abspath(__file__))
datafiles = glob.glob(dirname + '/*.data')

nodes = ["achal01", "achal02", "achal03", "achal04"]

kvs_type_bft = "BFTKVS"
kvs_type_redis = "RedisKVS2"
kvs_type_etcd = "ETCDKVS"

def get_graph_data(kvs_type_str):

  data = {}
  
  #for node in nodes:
  for data_file in datafiles:
    node = data_file.split('/')[-1].split('_')[0]

    #print("data_file", data_file)
    #print("node", node)
  
    #data_file = node + "_" + ts + ".data"
    arr = np.loadtxt(data_file, delimiter=",", dtype=str, skiprows=1)
  
    for row in arr:
  
      tasks_per_cpu = int(row[2])
      num_cpus = int(row[3])
      period = int(row[4])
      kvs_type = row[5]
      kvs_type = kvs_type[1:]
      node_id = int(row[6])
      sr = float(row[7])
      sw = float(row[8])
      sbr = float(row[9])
      sbw = float(row[10])
      app_bcet = float(row[11])
      app_acet = float(row[12])
      app_wcet = float(row[13])
      kvs_bcet = float(row[14])
      kvs_acet = float(row[15])
      kvs_wcet = float(row[16])
      stability = row[17]

      if kvs_type != kvs_type_str:
        continue
  
      assert("achal0" + str(node_id) == node)
  
      if num_cpus not in data: data[num_cpus] = {}
      if tasks_per_cpu not in data[num_cpus]: data[num_cpus][tasks_per_cpu] = {}
      if period not in data[num_cpus][tasks_per_cpu] : data[num_cpus][tasks_per_cpu][period] = {}
      if node not in data[num_cpus][tasks_per_cpu][period] : data[num_cpus][tasks_per_cpu][period][node] = {}
  
      #print("Adding data for %s, %s, %s, %s" % (num_cpus, tasks_per_cpu, period, node))
      data[num_cpus][tasks_per_cpu][period][node]["sr"] = sr
      data[num_cpus][tasks_per_cpu][period][node]["sw"] = sw
      data[num_cpus][tasks_per_cpu][period][node]["sbr"] = sbr
      data[num_cpus][tasks_per_cpu][period][node]["sbw"] = sbw
      data[num_cpus][tasks_per_cpu][period][node]["app_bcet"] = app_bcet
      data[num_cpus][tasks_per_cpu][period][node]["app_acet"] = app_acet
      data[num_cpus][tasks_per_cpu][period][node]["app_wcet"] = app_wcet
      data[num_cpus][tasks_per_cpu][period][node]["kvs_bcet"] = kvs_bcet
      data[num_cpus][tasks_per_cpu][period][node]["kvs_acet"] = kvs_acet
      data[num_cpus][tasks_per_cpu][period][node]["kvs_wcet"] = kvs_wcet
      data[num_cpus][tasks_per_cpu][period][node]["stability"] = stability
  
  for num_cpus in data:
    for tasks_per_cpu in data[num_cpus]:
      for period in data[num_cpus][tasks_per_cpu]:
        
        global_sr = 0.0
        global_sw = 0.0
        global_sbr = 0.0
        global_sbw = 0.0
        global_app_bcet = sys.float_info.max
        global_app_acet = 0.0
        global_app_wcet = 0.0
        global_kvs_bcet = sys.float_info.max
        global_kvs_acet = 0.0
        global_kvs_wcet = 0.0
        global_stability = True

        missing = 0
  
        for node in nodes: 
  
          #print("Retreiving data for %s, %s, %s, %s" % (num_cpus, tasks_per_cpu, period, node))
  
          try:
            global_sr += data[num_cpus][tasks_per_cpu][period][node]["sr"]
            global_sw += data[num_cpus][tasks_per_cpu][period][node]["sw"]
            global_sbr += data[num_cpus][tasks_per_cpu][period][node]["sbr"]
            global_sbw += data[num_cpus][tasks_per_cpu][period][node]["sbw"]
  
            if data[num_cpus][tasks_per_cpu][period][node]["app_bcet"] < global_app_bcet:
              global_app_bcet = data[num_cpus][tasks_per_cpu][period][node]["app_bcet"]
            if data[num_cpus][tasks_per_cpu][period][node]["app_wcet"] > global_app_wcet:
              global_app_wcet = data[num_cpus][tasks_per_cpu][period][node]["app_wcet"]
  
            if data[num_cpus][tasks_per_cpu][period][node]["kvs_bcet"] < global_kvs_bcet:
              global_kvs_bcet = data[num_cpus][tasks_per_cpu][period][node]["kvs_bcet"]
            if data[num_cpus][tasks_per_cpu][period][node]["kvs_wcet"] > global_kvs_wcet:
              global_kvs_wcet = data[num_cpus][tasks_per_cpu][period][node]["kvs_wcet"]
  
            global_app_acet += data[num_cpus][tasks_per_cpu][period][node]["app_acet"]
            global_kvs_acet += data[num_cpus][tasks_per_cpu][period][node]["kvs_acet"]
  
            if data[num_cpus][tasks_per_cpu][period][node]["stability"] == "failure": 
              global_stability = False

          except KeyError:
            missing += 1
  
        
        global_sr /= (len(nodes) - missing)
        global_sw /= (len(nodes) - missing)
        global_sbr /= (len(nodes) - missing)
        global_sbw /= (len(nodes) - missing)
  
        global_app_acet /= len(nodes)
        global_kvs_acet /= len(nodes)
  
        assert("cluster" not in data[num_cpus][tasks_per_cpu][period])
        data[num_cpus][tasks_per_cpu][period]["cluster"] = {}
  
        data[num_cpus][tasks_per_cpu][period]["cluster"]["sr"] = global_sr
        data[num_cpus][tasks_per_cpu][period]["cluster"]["sw"] = global_sw
        data[num_cpus][tasks_per_cpu][period]["cluster"]["sbr"] = global_sbr
        data[num_cpus][tasks_per_cpu][period]["cluster"]["sbw"] = global_sbw
        data[num_cpus][tasks_per_cpu][period]["cluster"]["app_bcet"] = global_app_bcet
        data[num_cpus][tasks_per_cpu][period]["cluster"]["app_acet"] = global_app_acet
        data[num_cpus][tasks_per_cpu][period]["cluster"]["app_wcet"] = global_app_wcet
        data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_bcet"] = global_kvs_bcet
        data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_acet"] = global_kvs_acet
        data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_wcet"] = global_kvs_wcet
        data[num_cpus][tasks_per_cpu][period]["cluster"]["stability"] = global_stability

  labels = []
  Y1 = []
  Y2 = []
  Y1_err_low = []
  Y1_err_high = []
  Y2_err_low = []
  Y2_err_high = []
  Y3 = []
  Y4 = []
  
  #for period in [800, 400, 200, 100, 50]:
  #  for num_cpus in [1, 3]:
  #    for tasks_per_cpu in [1, 4]:
  for num_cpus in data:
    for tasks_per_cpu in data[num_cpus]:
      for period in data[num_cpus][tasks_per_cpu]:
  
        config = "$C=%s$\n$I=%s$\n$T=%s$" % (num_cpus, tasks_per_cpu, period)
        #print(config, \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["sr"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["sw"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["sbr"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["sbw"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["app_bcet"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["app_acet"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["app_wcet"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_bcet"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_acet"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_wcet"], \
        #      data[num_cpus][tasks_per_cpu][period]["cluster"]["stability"])
  
        labels.append(config)
        Y1.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["app_acet"])
        Y2.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_acet"])
        Y1_err_low.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["app_acet"] - data[num_cpus][tasks_per_cpu][period]["cluster"]["app_bcet"])
        Y1_err_high.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["app_wcet"] - data[num_cpus][tasks_per_cpu][period]["cluster"]["app_acet"])
        Y2_err_low.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_acet"] - data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_bcet"])
        Y2_err_high.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_wcet"] - data[num_cpus][tasks_per_cpu][period]["cluster"]["kvs_acet"])
        Y3.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["sbr"])
        Y4.append(data[num_cpus][tasks_per_cpu][period]["cluster"]["sbw"])

  return [Y1, Y1_err_low, Y1_err_high, Y2, Y2_err_low, Y2_err_high, Y3, Y4, labels]

BFT_Y = get_graph_data(kvs_type_bft)
ETCD_Y = get_graph_data(kvs_type_etcd)
REDIS_Y = get_graph_data(kvs_type_redis)
labels = BFT_Y[-1]

width = 0.1

colorsg = plt.cm.Greens(np.linspace(0, 0.5, 5))
colorsb = plt.cm.Blues(np.linspace(0, 0.5, 5))
colorsr = plt.cm.Reds(np.linspace(0, 0.5, 5))

X = np.arange(len(BFT_Y[0]))
X = X * 0.725
plt.figure(figsize=(18,4))
plt.bar(X - 0.25, BFT_Y[3], width, yerr=[BFT_Y[4], BFT_Y[5]], label='In-ConcReTeS (KVS)', color=colorsg[-1])
plt.bar(X - 0.15, BFT_Y[0], width, yerr=[BFT_Y[1], BFT_Y[2]], label='In-ConcReTeS (IvPSim)', color=colorsg[-3])
plt.bar(X - 0.05, REDIS_Y[3], width, yerr=[REDIS_Y[4], REDIS_Y[5]], label='Redis (KVS)', color=colorsb[-1])
plt.bar(X + 0.05, REDIS_Y[0], width, yerr=[REDIS_Y[1], REDIS_Y[2]], label='Redis (IvpSim)', color=colorsb[-3])
plt.bar(X + 0.15, ETCD_Y[3], width, yerr=[ETCD_Y[4], ETCD_Y[5]], label='Etcd (KVS)', color=colorsr[-1])
plt.bar(X + 0.25, ETCD_Y[0], width, yerr=[ETCD_Y[1], ETCD_Y[2]], label='Etcd (IvPSim)', color=colorsr[-3])
plt.ylabel('Execution Times (ms)')
plt.yscale('log')
plt.ylim([1e-4, 1e4])
plt.xticks(X, labels)
#plt.title('ACETs for different configs (#CPUs x #Tasks/CPU, Period)')
plt.margins(x=0.01, tight=True)
ax = plt.gca()
ax.set_xticklabels(labels=labels,rotation=0)
plt.legend(ncol=6)
plt.savefig(dirname + '/et.pdf', bbox_inches='tight', transparent=True)
plt.clf()

#X = np.arange(len(Y3))
plt.figure(figsize=(18,4))
plt.bar(X - 0.25, BFT_Y[6], width, label='In-ConcReTeS (reads)', color=colorsg[-1])
plt.bar(X - 0.15, BFT_Y[7], width, label='In-ConcReTeS (writes)', color=colorsg[-3])
plt.bar(X - 0.05, REDIS_Y[6], width, label='Redis (reads)', color=colorsb[-1])
plt.bar(X + 0.05, REDIS_Y[7], width, label='Redis (writes)', color=colorsb[-3])
plt.bar(X + 0.15, ETCD_Y[6], width, label='ETCD (reads)', color=colorsr[-1])
plt.bar(X + 0.25, ETCD_Y[7], width, label='ETCD (writes)', color=colorsr[-3])
plt.ylabel('Successful Iterations (%)')
plt.yscale('log')
plt.ylim([1e-4, 1e3])
plt.xticks(X, labels)
#plt.title('Success rates for different configs (#CPUs x #Tasks/CPU, Period)')
plt.margins(x=0.01, tight=True)
ax = plt.gca()
ax.set_xticklabels(labels=labels,rotation=0)
plt.legend(ncol=6)
plt.savefig(dirname + '/s.pdf', bbox_inches='tight', transparent=True)
