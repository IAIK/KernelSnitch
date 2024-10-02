#!/bin/bash

echo hrtimer red-black tree with software-agnostic amplification
./hrtimer_bases_clock_base_active.elf 1 7 hrtimer_rbt_amp.csv &> /dev/null
sleep 2
echo hrtimer red-black tree without software-agnostic amplification
./hrtimer_bases_clock_base_active.elf 0 7 hrtimer_rbt_no_amp.csv &> /dev/null