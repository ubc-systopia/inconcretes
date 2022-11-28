import os
import sys
import glob
import numpy as np
import matplotlib.pyplot as plt
from statistics import mean

dirname = os.path.dirname(os.path.abspath(__file__))
datafiles = glob.glob(dirname + '/*.data.et')

nodes = ["achal01", "achal04"]
#nodes = ["achal05", "achal06", "achal07", "achal08"]

data = {}

#for node in nodes:
for data_file in datafiles:
  node = data_file.split('/')[-1].split('_')[0]
  if node not in nodes: continue

  #data_file = node + "-" + bft_ts + ".data.et"
  arr = np.loadtxt(data_file, delimiter=",", dtype=str)

  max_job_id = 0
  for row in arr:
    
    if row[0] == "Job":
      continue
    
    job = int(row[0])
    #if job > max_job_id:
    #  max_job_id = job

    if max_job_id not in data:
      data[max_job_id] = {}
      data[max_job_id]["IvPSim"] = set()
      data[max_job_id]["KVS"] = set()

    for col in range(1, 5):
      data[max_job_id]["IvPSim"].add(float(row[col]))
    for col in range(5, 6):
      data[max_job_id]["KVS"].add(float(row[col]))

    max_job_id += 1

X = []
Y1 = []
Y1_min = []
Y1_max = []
Y2 = []
Y2_min = []
Y2_max = []

for job in range(2000, max_job_id):
  X.append(job - 2000)
  Y1.append(mean(data[job]["IvPSim"]))
  Y1_min.append(min(data[job]["IvPSim"]))
  Y1_max.append(max(data[job]["IvPSim"]))
  Y2.append(mean(data[job]["KVS"]))
  Y2_min.append(min(data[job]["KVS"]))
  Y2_max.append(max(data[job]["KVS"]))

colorsg = plt.cm.Greens(np.linspace(0, 0.5, 5))
colorsb = plt.cm.Blues(np.linspace(0, 0.5, 5))
colorsr = plt.cm.Reds(np.linspace(0, 0.5, 5))

plt.figure(figsize=(9,4))

for i in range(1, 6):
  for k in range(0, 1999):
    color='lightgreen' #colorsg[-3]
    if i == 3 or i == 5:
      color='lightgrey' #colorsr[-3]
      plt.vlines(2000 * (i - 1) + k, 1e-2, 1e2, color=color)

for i in range(1, 6):
  plt.vlines(2000 * i - 1, 1e-2, 1e2, color='grey', linestyle='dashed')

plt.plot(X, Y2, color='blue', label='KVS')
#plt.fill_between(X, Y2_min, Y2_max, alpha=0.2, color='red', label='KVS BCET-WCET (In-ConcReTeS)')
plt.plot(X, Y1, color='red', label='IvPSim')
#plt.fill_between(X, Y1_min, Y1_max, alpha=0.2, color='blue', label='IvPSim BCET-WCET (In-ConcReTeS)')
plt.ylabel('Execution Time (ms)')
plt.xlabel('Job IDs')
plt.yscale('log')
plt.ylim([1e-2, 1e2])
#plt.xticks(X, labels)
##plt.title('ACETs for different configs (#CPUs x #Tasks/CPU, Period)')
plt.margins(x=0.0, tight=True)
#ax = plt.gca()
x1 = range(0, 10001, 2000)
plt.xticks(x1)
plt.legend(ncol=1)
plt.savefig(dirname + '/et_profile.pdf', bbox_inches='tight', transparent=True)
