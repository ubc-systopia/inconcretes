# Run as 'python3 plot_ptp.py'

import pandas as pd
import matplotlib.pyplot as plt

for id in ['01', '02', '03', '04']:

  try:
    df = pd.read_csv('ptp' + id + '.dat', sep=' ', header=0, names=['time', 'offset', 'freq', 'pathDelaay'], index_col=False)
  except FileNotFoundError:
    continue

  df['offset'] /= 1000
  
  df.plot.line(x='time', y='offset', title="PTP Clock Offset on Node " + id, figsize=(18, 6))
  plt.xlabel('Time (seconds)')
  plt.ylabel('Offset (us)')
  plt.ylim([-500, 500])
  plt.savefig('output' + id + '.png', bbox_inches='tight')
