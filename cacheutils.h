#ifndef CACHEUTILS_H
#define CACHEUTILS_H

#define _GNU_SOURCE
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef HIDEMINMAX
#define MAX(X,Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X,Y) (((X) < (Y)) ? (X) : (Y))
#endif

void pin_to_core(size_t core)
{
    int ret;
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(core, &cpuset);

    ret = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (ret) {
      perror("sched_setaffinity: ");
      exit(-1);
    }
}

void reset_cpu_pin(void)
{
    cpu_set_t cpuset;
    memset(&cpuset, 0xff, sizeof(cpu_set_t));
    int ret;
    ret = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    if (ret) {
      perror("sched_setaffinity: ");
      exit(-1);
    }
}

#define INTEL
// #define AMD

#define RDPRU ".byte 0x0f, 0x01, 0xfd"
#define RDPRU_ECX_MPERF	0
#define RDPRU_ECX_APERF	1

static inline size_t rdtsc_begin(void)
{
#if defined(INTEL)
  size_t a, d;
  asm volatile ("mfence");
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));
  a = (d<<32) | a;
  asm volatile ("lfence");
  return a;
#elif defined(AMD)
  unsigned long low_a, high_a;
  asm volatile ("mfence");
  asm volatile(RDPRU
			     : "=a" (low_a), "=d" (high_a)
			     : "c" (RDPRU_ECX_APERF));
  unsigned long aval = ((low_a) | (high_a) << 32);
  asm volatile ("lfence");
  return aval;
#endif
}

static inline size_t rdtsc_end(void)
{
#if defined(INTEL)
  size_t a, d;
  asm volatile ("lfence");
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));
  a = (d<<32) | a;
  asm volatile ("mfence");
  return a;
#elif defined(AMD)
  unsigned long low_a, high_a;
  asm volatile ("lfence");
  asm volatile(RDPRU
			     : "=a" (low_a), "=d" (high_a)
			     : "c" (RDPRU_ECX_APERF));
  unsigned long aval = ((low_a) | (high_a) << 32);
  asm volatile ("mfence");
  return aval;
#endif
}

static inline void flush(__attribute__((unused))size_t p)
{
  asm volatile (".intel_syntax noprefix");
  asm volatile ("clflush qword ptr [%0]\n" : : "r" (p));
  asm volatile (".att_syntax");
}

static inline void prefetch(__attribute__((unused))size_t p)
{
  asm volatile (".intel_syntax noprefix");
  asm volatile ("prefetchnta qword ptr [%0]" : : "r" (p));
  asm volatile ("prefetcht2 qword ptr [%0]" : : "r" (p));
  asm volatile (".att_syntax");
}

static inline void longnop(void)
{
  asm volatile ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
                "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
}
#endif
