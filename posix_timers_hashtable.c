// #define MEMSET
#include "cacheutils.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define TOTAL_TIMERS 512
#define LOOP 8
#define TIMERS 256

int __create_timer(void)
{
    int ret = 0;
    size_t timer_id = 0;
    ret = timer_create(0, 0, (timer_t *)&timer_id);
    if (ret < 0) {
        perror("timer_create: ");
        return -1;
    }
    return (int)timer_id;
}
void __delete_timer(size_t timer_id)
{
    int ret = 0;
    ret = timer_delete((timer_t)timer_id);
    if (ret != 0) {
        perror("timer_delete: ");
        exit(-1);
    }
}

void __prime_timer(size_t timer_id)
{
    int ret = timer_gettime((timer_t)timer_id, 0);
    if (ret == 0) {
        perror("timer_gettime: ");
        exit(-1);
    }
}
int compare(const void *a, const void *b)
{
  return (*(size_t *)a - *(size_t *)b);
}
#define REPEAT_MEASUREMENT 512//(1<<6)
#define AVERAGE (1<<3)
size_t software_agnostic_amplification = 0;
size_t prime_timer(size_t timer_id)
{
    static volatile char flush_cache[(1<<10)*(1<<7)]; 
    size_t t0;
    size_t t1;
    size_t time = 0;
    size_t times[REPEAT_MEASUREMENT];
    for (size_t i = 0; i < REPEAT_MEASUREMENT; ++i) {
        sched_yield();
        if (software_agnostic_amplification)
            memset((char *)flush_cache, 1, sizeof(flush_cache));
        t0 = rdtsc_begin();
        __prime_timer(timer_id);
        t1 = rdtsc_end();
        times[i] = t1 - t0;
    }
    qsort(times, REPEAT_MEASUREMENT, sizeof(size_t), compare);
    for (size_t i = 0; i < AVERAGE; ++i)
        time += times[i];
    time /= AVERAGE;
    return time;
}

#define INDEX (1024*1024-1) // is invalid as no timer can have this index
void __print_ground_truth(size_t with_score)
{
    int ret;
    size_t count;
    size_t counts[512];
    for (size_t i = 0; i < 512; ++i) {
        ret = timer_read_count_hash((size_t)&count, INDEX+i);
        if (ret != 0) {
            printf("[!] timer %zd error\n", i);
            exit(-1);
        }
        counts[i] = count;
    }
    size_t highest_count = 0;
    for (size_t i = 0; i < 512; ++i) {
        if (counts[i] > highest_count) {
            highest_count = counts[i];
        }
    }
    for (size_t i = 0; i < 512; ++i) {
        if (i % 32 == 0)
            printf("[%4zd] ", i);
        if (with_score == 1 && counts[i] == highest_count)
            printf("\033[1;31m");
        printf("%4zd", counts[i]);
        if (with_score == 1 && counts[i] == highest_count)
            printf("\033[0m");
        if (i % 32 == 31)
            printf("\n");
    }
}
void print_ground_truth(void)
{
    __print_ground_truth(0);
}
void print_ground_truth_score(void)
{
    __print_ground_truth(1);
}
void __print_side_channel(size_t with_score)
{
    size_t times[512];
    for (size_t i = 0; i < 512; ++i)
        times[i] = prime_timer(INDEX+i);
    size_t highest_time = 0;
    for (size_t i = 0; i < 512; ++i) {
        if (times[i] > highest_time) {
            highest_time = times[i];
        }
    }
    for (size_t i = 0; i < 512; ++i) {
        if (i % 32 == 0)
            printf("[%4zd] ", i);
        if (with_score == 1 && times[i] == highest_time)
            printf("\033[1;31m");
        printf("%4zd", times[i]);
        if (with_score == 1 && times[i] == highest_time)
            printf("\033[0m");
        if (i % 32 == 31)
            printf("\n");
    }
}
void print_side_channel(void)
{
    __print_side_channel(0);
}
void print_side_channel_score(void)
{
    __print_side_channel(1);
}
void print_side_channel_and_ground_truth(void)
{
    int ret;
    size_t ground_truth;
    size_t ground_truths[512];
    size_t times[512];
    for (size_t i = 0; i < 512; ++i) {
        times[i] = prime_timer(INDEX+i);
        ret = timer_read_count_hash((size_t)&ground_truth, INDEX+i);
        if (ret != 0) {
            printf("[!] timer %zd error\n", i);
            exit(-1);
        }
        ground_truths[i] = ground_truth;
    }
    for (size_t i = 0; i < 512; ++i) {
        if (i % 16 == 0)
            printf("[%4zd] ", i);
        printf("%4zd (%zd)", times[i], ground_truths[i]);
        if (i % 16 == 15)
            printf("\n");
    }
}

size_t read_hash(size_t timer_id)
{
    int ret;
    size_t hash;
    ret = timer_read_index_hash((size_t)&hash, timer_id);
    if (ret != 0) {
        printf("[!] timer_read_index_hash %zd error\n", timer_id);
        exit(-1);
    }
    return hash;
}

void print_usage(char *name)
{
    printf("%s <struct_agnostic_amp=0> <core=7> <file_name=futex_hash_table_%%s.csv>\n", name);
    exit(0);
}

char *name = 0;
size_t core = 7;
int main(int argc, char **argv)
{
    if (argc > 4)
        print_usage(argv[0]);
    software_agnostic_amplification = (argc > 1) && strcmp(argv[1], "0");
    if (argc > 2)
        core = atoi(argv[2]);
    if (argc > 3)
        name = argv[3];

    helper_init();
    pin_to_core(core);

    int ret;
    size_t ground_truths[TOTAL_TIMERS*LOOP];
    size_t times[TOTAL_TIMERS*LOOP];
    printf("[*] warmup\n");
    for (volatile size_t i = 0; i < 0x100000000; ++i);
    print_side_channel_and_ground_truth();
    printf("[*] start\n");
    for (size_t j = 0; j < LOOP; ++j) {
        printf("[*] %zd/%d\n", j, LOOP);
        for (size_t i = 0; i < TIMERS; ++i)
            __create_timer();
        size_t ground_truth;
        for (size_t i = 0; i < TOTAL_TIMERS; ++i) {
            times[i+j*TOTAL_TIMERS] = prime_timer(INDEX+i);
            ret = timer_read_count_hash((size_t)&ground_truth, INDEX+i);
            if (ret != 0) {
                printf("[!] timer %zd error\n", i);
                exit(-1);
            }
            ground_truths[i+j*TOTAL_TIMERS] = ground_truth;
        }
    }
    for (size_t i = 0; i < TOTAL_TIMERS*LOOP; ++i) {
        if (i % 32 == 0)
            printf("[%4zd] ", i);
        printf("%4zd (%zd)", times[i], ground_truths[i]);
        if (i % 32 == 31)
            printf("\n");
    }

    if (name == 0) {
        printf("[?] save to file? [y/N] ");
        char in = getchar();
        if (in != 'y')
            exit(0);
    }

    time_t current_time;
    struct tm *timeinfo;
    char buffer[256];

    time(&current_time);
    timeinfo = localtime(&current_time);
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);

    if (name == 0) {
        static char file_name[0x1000];
        snprintf(file_name, sizeof(file_name), "posix_timers_hashtable_%s.csv", buffer);
        name = file_name;
    }
    FILE *csv_file = fopen(name, "w");

    if (csv_file == NULL) {
        printf("[!] fopen\n");
        return 1;
    }

    // Write headers
    fprintf(csv_file, "ground truth; time\n");

    for (size_t i = 0; i < TOTAL_TIMERS*LOOP; ++i) {
        fprintf(csv_file, "%ld; %ld\n", ground_truths[i], times[i]);
    }

    fclose(csv_file);
    printf("[+] done\n");

}
