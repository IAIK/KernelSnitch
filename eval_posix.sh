#!/bin/bash

echo posix timer hash table with software-agnostic amplification
./posix_timers_hashtable.elf 1 7 posix_ht_amp.csv &> /dev/null
sleep 2
echo posix timer hash table without software-agnostic amplification
./posix_timers_hashtable.elf 0 6 posix_ht_no_amp.csv &> /dev/null