#!/usr/bin/env python3

import csv
import argparse
import matplotlib.pyplot as plt
import numpy as np

parser = argparse.ArgumentParser(description="Prints the improvements for hrtimers")
parser.add_argument("filename_no_amp", help="Name of the csv file without amp")
parser.add_argument("filename_amp", help="Name of the csv file with amp")

args = parser.parse_args()
with open(args.filename_no_amp, "r") as f:
    csv_reader = csv.reader(f, delimiter=";")
    row = list(csv_reader)[1:]
    data_no_amp = {int(r[0]): int(r[1]) for r in row}
    min_no_amp = min(data_no_amp.keys())
with open(args.filename_amp, "r") as f:
    csv_reader = csv.reader(f, delimiter=";")
    row = list(csv_reader)[1:]
    data_amp = {int(r[0]): int(r[1]) for r in row}
    min_amp = min(data_amp.keys())

plt.plot(data_no_amp.values())
plt.plot(data_amp.values())
plt.show()
