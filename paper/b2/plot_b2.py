import os
import sys
import glob
import numpy as np
import matplotlib.pyplot as plt

dirname = os.path.dirname(os.path.abspath(__file__))
datafiles = glob.glob(dirname + '/*.data')

nodes = ["achal01", "achal02", "achal03", "achal04"]

outfile = open(dirname + "/table.txt", "w")

def get_graph_data():

  data = {}
  
  #for node in nodes:
  for data_file in datafiles:
    node = data_file.split('/')[-1].split('_')[0]
  
    #data_file = node + "_" + ts + ".data"
    arr = np.loadtxt(data_file, delimiter=",", dtype=str, skiprows=1)
 
    for row in arr:
  
      config_file = row[1]

      tasks_per_cpu = int(row[2])
      assert(tasks_per_cpu == 4 or tasks_per_cpu == 3)

      labels = int(row[3])

      num_cpus = int(row[7])
      assert(num_cpus == 1)

      period = int(row[8])
      assert(period == 50 or period == 100)

      kvs_type = row[9]
      kvs_type = kvs_type[1:]
      assert (kvs_type == "BFTKVS")

      node_id = int(row[10])

      sr = float(row[11])
      sw = float(row[12])
      sbr = float(row[13])
      sbw = float(row[14])

      app_bcet = float(row[15])
      app_acet = float(row[16])
      app_wcet = float(row[17])
      kvs_bcet = float(row[18])
      kvs_acet = float(row[19])
      kvs_wcet = float(row[20])
  
      assert("achal0" + str(node_id) == node)

      #print(config_file, labels)
  
      #print("Adding data for %s, %s" % (config_file, node))
      if config_file not in data: data[config_file] = {}
      if node not in data[config_file]: data[config_file][node] = {}
      data[config_file][node]["labels"] = labels
      data[config_file][node]["sr"] = sr
      data[config_file][node]["sw"] = sw
      data[config_file][node]["sbr"] = sbr
      data[config_file][node]["sbw"] = sbw
      data[config_file][node]["app_bcet"] = app_bcet
      data[config_file][node]["app_acet"] = app_acet
      data[config_file][node]["app_wcet"] = app_wcet
      data[config_file][node]["kvs_bcet"] = kvs_bcet
      data[config_file][node]["kvs_acet"] = kvs_acet
      data[config_file][node]["kvs_wcet"] = kvs_wcet

  for config_file in data:
    
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
    global_labels = None

    missing = 0
  
    for node in nodes: 
  
      #print("Retreiving data for %s, %s" % (config_file, node))
  
      try:
        if global_labels == None:
          global_labels = data[config_file][node]["labels"]
        else:
          assert(global_labels == data[config_file][node]["labels"])
          
        global_sr +=  data[config_file][node]["sr"]
        global_sw +=  data[config_file][node]["sw"]
        global_sbr += data[config_file][node]["sbr"]
        global_sbw += data[config_file][node]["sbw"]
  
        if data[config_file][node]["app_bcet"] < global_app_bcet:
          global_app_bcet = data[config_file][node]["app_bcet"]
        if data[config_file][node]["app_wcet"] > global_app_wcet:
          global_app_wcet = data[config_file][node]["app_wcet"]
  
        if data[config_file][node]["kvs_bcet"] < global_kvs_bcet:
          global_kvs_bcet = data[config_file][node]["kvs_bcet"]
        if data[config_file][node]["kvs_wcet"] > global_kvs_wcet:
          global_kvs_wcet = data[config_file][node]["kvs_wcet"]
  
        global_app_acet += data[config_file][node]["app_acet"]
        global_kvs_acet += data[config_file][node]["kvs_acet"]
  
      except KeyError:
        print("KeyError")
        missing += 1

    global_sr /= (len(nodes) - missing)
    global_sw /= (len(nodes) - missing)
    global_sbr /= (len(nodes) - missing)
    global_sbw /= (len(nodes) - missing)
  
    global_app_acet /= len(nodes)
    global_kvs_acet /= len(nodes)
  
    assert("cluster" not in data[config_file])
    data[config_file]["cluster"] = {}
  
    data[config_file]["cluster"]["labels"] = global_labels
    data[config_file]["cluster"]["sr"] = global_sr
    data[config_file]["cluster"]["sw"] = global_sw
    data[config_file]["cluster"]["sbr"] = global_sbr
    data[config_file]["cluster"]["sbw"] = global_sbw
    data[config_file]["cluster"]["app_bcet"] = global_app_bcet
    data[config_file]["cluster"]["app_acet"] = global_app_acet
    data[config_file]["cluster"]["app_wcet"] = global_app_wcet
    data[config_file]["cluster"]["kvs_bcet"] = global_kvs_bcet
    data[config_file]["cluster"]["kvs_acet"] = global_kvs_acet
    data[config_file]["cluster"]["kvs_wcet"] = global_kvs_wcet

  config_file_labels = {}
  for config_file in data:
    config_file_labels[config_file] = data[config_file]["cluster"]["labels"]
  config_file_labels = sorted(config_file_labels.items(), key=lambda x:x[1])

  file_id = 0
  for (config_file, label) in config_file_labels:
    file_id += 1
    #print("%d & %.2f & %.2f & %0.2f & %0.2f & %0.2f & %0.2f \\\\" % ( \
    #  data[config_file]["cluster"]["labels"], \
    #  data[config_file]["cluster"]["sbr"], \
    #  data[config_file]["cluster"]["sbw"], \
    #  data[config_file]["cluster"]["app_acet"], \
    #  data[config_file]["cluster"]["app_wcet"], \
    #  data[config_file]["cluster"]["kvs_acet"], \
    #  data[config_file]["cluster"]["kvs_wcet"] ))
    outfile.write("%d & %.2f & %.2f & %0.2f & %0.2f & %0.2f & %0.2f \\\\" % ( \
      data[config_file]["cluster"]["labels"], \
      data[config_file][nodes[0]]["sbr"], \
      data[config_file][nodes[1]]["sbr"], \
      data[config_file][nodes[2]]["sbr"], \
      data[config_file][nodes[3]]["sbr"], \
      data[config_file]["cluster"]["kvs_acet"], \
      data[config_file]["cluster"]["kvs_wcet"] ))
    outfile.write("\n")

      #1 & 1000 & 84.44\% & 80.44\% & 1.23 ms & 23.45 ms \\

get_graph_data()

outfile.close()
