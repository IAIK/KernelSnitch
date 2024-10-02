#!/usr/bin/env python3

import csv
import numpy as np

def eval(filename, gt0, gt1):
    with open(filename, "r") as f:
        csv_reader = csv.reader(f, delimiter=";")
        row = list(csv_reader)[1:]
        data = np.array([[int(r[0]), int(r[1])] for r in row])
        data0 = [d[1] for d in data if d[0] == int(gt0)]
        data1 = [d[1] for d in data if d[0] == int(gt1)]
        mean0 = np.median(data0)
        mean1 = np.median(data1)
        threshold = (mean1 + mean0)/2
        fp = np.count_nonzero(data1 < threshold)
        fn = np.count_nonzero(data0 > threshold)
        p = (fp+fn)/(len(data))
        # print("data0.median()  {: 5.1f}".format(mean0))
        # print("data1.median()  {: 5.1f}".format(mean1))
        # print("threshold       {: 5.1f}".format(threshold))
        # print("data0           {: 5d}".format(np.count_nonzero(data0)))
        # print("data1           {: 5d}".format(np.count_nonzero(data1)))
        # print("false positives {: 5d} {: 3.1f}%".format(fp, 100*fp/np.count_nonzero(data1)))
        # print("false negatives {: 5d} {: 3.1f}%".format(fn, 100*fn/np.count_nonzero(data0)))
        fp=100*fp/np.count_nonzero(data1)
        fn=100*fn/np.count_nonzero(data0)
        return fp,fn,p

'''
futex_ht_no_amp.csv
futex_ht_amp.csv
posix_ht_no_amp.csv
posix_ht_amp.csv
ipc_ht_no_amp.csv
ipc_ht_amp.csv
ipc_rt_no_amp.csv
ipc_rt_amp.csv
timer_rbt_no_amp.csv
timer_rbt_amp.csv
'''
def eval_print(name, filename0, filename1):
    fp0,fn0,_ = eval(filename0, 0, 1)
    fp1,fn1,_ = eval(filename0, 0, 3)
    fp2,fn2,_ = eval(filename1, 0, 1)
    fp3,fn3,_ = eval(filename1, 0, 3)
    if fn0 != 0:
        fn_reduce = 100.0*(1-fn3/fn0)
    else:
        fn_reduce = 100.0
    if fp0 != 0:
        fp_reduce = 100.0*(1-fp3/fp0)
    else:
        fp_reduce = 100.0
    fp0 = "{:5.1f}".format(fp0)
    fn0 = "{:5.1f}".format(fn0)
    fp1 = "{:5.1f}".format(fp1)
    fn1 = "{:5.1f}".format(fn1)
    fp2 = "{:5.1f}".format(fp2)
    fn2 = "{:5.1f}".format(fn2)
    fp3 = "{:5.1f}".format(fp3)
    fn3 = "{:5.1f}".format(fn3)
    fp_reduce = "{:4.0f}".format(fp_reduce)
    fn_reduce = "{:4.0f}".format(fn_reduce)
    print("| {} | {} | {} | {} | {} | {} | {} | {} | {} | {} | {} |".format(name, fp0, fn0, fp1, fn1, fp2, fn2, fp3, fn3, fp_reduce, fn_reduce))
    pass
def eval_print1(name, filename0, filename1):
    fp0,fn0,_ = eval(filename0, 0, 1)
    fp2,fn2,_ = eval(filename1, 0, 1)
    if fn0 != 0:
        fn_reduce = 100*(1-fn2/fn0)
    else:
        fn_reduce = 100.0
    if fp0 != 0:
        fp_reduce = 100*(1-fp2/fp0)
    else:
        fp_reduce = 100.0
    fp0 = "{:5.1f}".format(fp0)
    fn0 = "{:5.1f}".format(fn0)
    fp2 = "{:5.1f}".format(fp2)
    fn2 = "{:5.1f}".format(fn2)
    fp_reduce = "{:4.0f}".format(fp_reduce)
    fn_reduce = "{:4.0f}".format(fn_reduce)
    print("| {} | {} | {} |       |       | {} | {} |       |       | {} | {} |".format(name, fp0, fn0, fp2, fn2, fp_reduce, fn_reduce))
    pass

print("-"*105)
print("| Container instance      |      W/o struct-agnostic      |      W struct-agnostic        |  Reduction  |")
print("|                         |     1 to 0    |    3 to 0     |     1 to 0    |    3 to 0     |             |")
print("|                         |  FPR  |  FNR  |  FPR  |  FNR  |  FPR  |  FNR  |  FPR  |  FNR  |             |")
print("-"*105)
eval_print("posix timers hash table", "posix_ht_no_amp.csv", "posix_ht_amp.csv")
eval_print("futex hash table       ", "futex_ht_no_amp.csv", "futex_ht_amp.csv")
eval_print("ipc hash table         ", "ipc_ht_no_amp.csv", "ipc_ht_amp.csv")
eval_print1("ipc radix tree         ", "ipc_rt_no_amp.csv", "ipc_rt_amp.csv")
print("-"*105)