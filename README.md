# KernelSnitch Artifacts

This repo contains artifacts developed during a research project, as well as the code to perform KernelSnitch, which got accepted at NDSS '25.

## What is KernelSnitch

KernelSnitch is a novel software-induced side-channel attack that targets kernel data container structures such as hash tables and trees. These structures vary in size and access time depending on the number of elements they hold, i.e., the occupancy level. KernelSnitch exploits this variability to constitute a timing side channel that is exploitable to an unprivileged, isolated attacker from user space. Despite the small timing differences relative to system call runtime, we demonstrate methods to reliably amplify these timing variations for successful exploitation.

The artifacts demonstrate the timing side channel and show the practicality of distinguishing between different occupancy levels. We provide a kernel module and execution scripts for evaluation. While our timing side channel is software induced, we recommend evaluation on hardware similar to ours (i.e., Intel i7-1260P, i7-1165G7, i7-12700, and Xeon Gold 6530) to reproduce similar results as in our paper. While the attacks should work generically on Linux kernels, we recommend to evaluate the artifacts on downstream Ubuntu Linux kernels v5.15, v6.5, or v6.8, as these are the versions we primarily evaluate. For the timing side channel, the evaluation shows that the occupancy level of data container structures can be leaked by measuring the timing of syscalls that access these structures.

## Description & Requirements

### Security, Privacy, and Ethical Concerns

The artifacts do not perform any destructive steps, as we only show the timing side channel to leak the occupancy level of data container structures and exclude the case studies, i.e., secretly transmitting data via a covert channel, leaking kernel heap pointers and monitoring user activity via website fingerprinting.

### How to Access

We provide the source code ([github](https://github.com/IAIK/KernelSnitch/)) for performing the timing side channel.

### Hardware Dependencies

While the timing side channel is software induced, one of the amplification methods depends on hardware buffers, i.e., the CPU caches. We have evaluated the Intel i7-1260P, i7-1165G7, i7-12700 and Xeon Gold 6530. We therefore expect similar results with similar processors.

### Software Dependencies

While the attacks should generically work on Linux kernels, we recommend evaluating the artifacts on a downstream Ubuntu Linux kernel v5.15, v6.5, or v6.8. For reference, our primary evaluation system was Ubuntu 22.04 with generic kernels v5.15, v6.5, or v6.8.

One part of the artifact evaluation is to insert a kernel module that requires *root privileges*. This module is required to obtain the ground truth of the occupancy level of kernel data structures. We tested our kernel module with the downstream Ubuntu Linux kernels v5.15, v6.5, and v6.8. For other kernels that have different config files (or other downstream changes) our implemented module may not do what we intended. Specifically, in order to obtain the occupancy ground truth of the data structures, we redefined and reimplemented several structures and functions according to how they are implemented in the Ubuntu Linux kernel. We did this because several functions used to access kernel data structures are implemented as inline functions, e.g., `__rhashtable_lookup`, which prevented us from calling these functions directly. Another reason is that several structs, e.g., `msg_queue`, are defined in c files, which also prevents us from using these struct definitions.

## Artifact Installation & Configuration

### Installation

The installation required to perform the artifact evaluation works as following:

 - Clone our [github repository](https://github.com/IAIK/KernelSnitch/) to the `/repo/path` directory.
 - Change directory to `/repo/path/modules`.
 - Execute `make init` to build and insert the kernel module.
 - Change directory to `/repo/path`.
 - Select in `/repo/path/cacheutils.h` either the `INTEL` or `AMD` macro depending on your system.
 - Execute `make` to build all experiment binaries.

### Basic Tests

Testing the basic functionality works as following:

 - Change directory to `/repo/path`.
 - Execute `./basic_test.elf` should print `[+] basic test passed`.

## Evaluation

As described in Section V-B **External Noise**, the most dominant noise source is CPU frequency fluctuation. Therefore, perform the following experiments with as little background activity as possible to reproduce the figures from the paper. We even suggest to perform the experiments on an idle system with no other activity.

#### POSIX timer hash table experiment (E1)

_How to:_
Execute `./posix_timers_hashtable.elf <struct_agnostic_amp> <core> <file_name>`, with `<struct_agnostic_amp>` is a boolean which performs the experiment with/without structure-agnostic amplification, `<core>` pins the process to the specific core, and `<file_name>` stores the results in this file. For convenience, we provide the `eval_posix.sh` script which internally executes `posix_timers_hashtable.elf` with and without structure-agnostic amplification. To reproduce Figure 8, execute `./print_hist.py -f <file_name>`, where `<file_name>` is either `posix_ht_amp.csv` or `posix_ht_no_amp.csv`.

_Preparation:_
Do **Installation**.

_Execution:_
`./eval_posix.sh` and `./print_hist.py -f <file_name>`, where `<file_name>` is either `posix_ht_amp.csv` or `posix_ht_no_amp.csv`.

_Results:_
`print_hist.py` should reproduce Figure 8.


#### Futex hash table experiment (E2)

_How to:_
Same as for (E1) but with `futex_hash_table.elf` and `eval_futex.sh`.

_Preparation:_
Do **Installation**.

_Execution:_
`./eval_futex.sh` and `./print_hist.py -f <file_name>`, where `<file_name>` is either `futex_ht_amp.csv` or `futex_ht_no_amp.csv`.

_Results:_
`print_hist.py` should reproduce Figure 9.


#### IPC hash table experiment (E3)

_How to:_
Same as for (E1) but with `ipc_ids_key_ht.elf` and `eval_ipc_ht.sh`.

_Preparation:_
Do **Installation**.

_Execution:_
`./eval_ipc_ht.sh` and `./print_hist.py -f <file_name>`, where `<file_name>` is either `ipc_ht_amp.csv` or `ipc_ht_no_amp.csv`.

_Results:_
`print_hist.py` should reproduce Figure 10.


#### IPC radix tree experiment (E4)

_How to:_
Same as for (E1) but with `ipc_ids_ipcs_idr_root_rt.elf` and `eval_ipc_rt.sh`.

_Preparation:_
Do **Installation**.

_Execution:_
`./eval_ipc_rt.sh` and and `./print_hist.py -f <file_name>`, where `<file_name>` is either `ipc_rt_amp.csv` or `ipc_rt_no_amp.csv`.

_Results:_
`print_hist.py` should reproduce Figure 11.


#### Hrtimer red-black tree experiment (E5)

_How to:_
Similar as for (E1) but with `hrtimer_bases_clock_base_active.elf` and `eval_hrtimer_rbt.sh`, and `print_hrtimer.py`.

_Preparation:_
Do **Installation**.

_Execution:_
`./eval_hrtimer_rbt.sh` and `./print_hrtimer.py hrtimer_rbt_no_amp.csv hrtimer_rbt_amp.csv`.

_Results:_
`print_hrtimer.py` should reproduce Figure 12.


#### Amplification Improvement experiment (E6)

_How to:_
Execute `eval.py` prints similar results to Table I in ASCII form.

_Preparation:_
Do **Installation** and experiments (E1-5).

_Execution:_
`./eval.py`.

_Results:_
`eval.py` should reproduce Table I.