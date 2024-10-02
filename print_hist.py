#!/usr/bin/env python3

import csv
import matplotlib.pyplot as plt
import numpy as np
import os
import argparse

def plot_histogram(filename, data, counts):
    legend = []
    for i in range(min(counts),10):
        d = data[counts == i]
        if len(d) == 0:
            continue
        max_value = np.max(d)
        min_value = np.min(d)
        bins = max_value-min_value
        if bins == 0:
            bins = 1
        plt.hist(d, bins=bins)
        legend.append(str(i))
        i += 1
    plt.legend(legend)
    plt.title(filename)
    plt.show()

def plot(filename):
    with open(filename, "r") as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=";")
        row = list(csv_reader)[1:]
        data = [[int(r[0]), int(r[1])] for r in row]
        counts = np.array([d[0] for d in data])
        data = np.array([d[1] for d in data])
        plot_histogram(filename, data, counts)

def plot_all():
    files = [f for f in os.listdir("./") if ".csv" in f]
    for f in files:
        print("file {}".format(f))
        with open(f, "r") as csv_file:
            csv_reader = csv.reader(csv_file, delimiter=";")
            row = list(csv_reader)[1:]
            data = [[int(r[0]), int(r[1])] for r in row]
            counts = np.array([d[0] for d in data])
            data = np.array([d[1] for d in data])
            plot_histogram(f, data, counts)

parser = argparse.ArgumentParser(description="Prints histogramm of csv file")
parser.add_argument("-f", "--filename", help="Name of the csv file", default="")
args = parser.parse_args()

if args.filename == "":
    plot_all()
else:
    plot(args.filename)