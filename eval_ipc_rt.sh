#!/bin/bash

echo ipc radix tree with software-agnostic amplification
./ipc_ids_ipcs_idr_root_rt.elf 1 7 ipc_rt_amp.csv &> /dev/null
sleep 2
echo ipc radix tree without software-agnostic amplification
./ipc_ids_ipcs_idr_root_rt.elf 0 6 ipc_rt_no_amp.csv &> /dev/null