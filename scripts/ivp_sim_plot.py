#!/usr/bin/python3
import argparse
import pandas as pd
import matplotlib.pyplot as plt

ap = argparse.ArgumentParser()
ap.add_argument("-i", "--tracefile", required=True, help="Input CSV file")
ap.add_argument("-o", "--plotfile", required=True, help="Output PDF file")
args = vars(ap.parse_args())

def plot_time_series_data(df):
  df.plot.line(x="time_tag", y="angular_position", sub)
  df.plot.line(x="time_tag", y="force")
  df.plot.line(x="time_tag", y="pre_error")
  df.plot.line(x="time_tag", y="integral")

if __name__ == "__main__":
  df = pd.read_csv(args['tracefile'])
  plot_time_series_data(df)
  plt.show()
