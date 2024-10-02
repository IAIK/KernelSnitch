#!/bin/bash

echo ipc hash table with software-agnostic amplification
./ipc_ids_key_ht.elf 1 7 ipc_ht_amp.csv &> /dev/null
sleep 2
echo ipc hash table without software-agnostic amplification
./ipc_ids_key_ht.elf 0 6 ipc_ht_no_amp.csv &> /dev/null