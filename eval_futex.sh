#!/bin/bash

echo futex hash table with software-agnostic amplification
./futex_hash_table.elf 1 7 futex_ht_amp.csv &> /dev/null
sleep 2
echo futex hash table without software-agnostic amplification
./futex_hash_table.elf 0 6 futex_ht_no_amp.csv &> /dev/null