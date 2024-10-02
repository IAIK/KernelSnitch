#include "cacheutils.h"
#include "helper.h"
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define TOTAL_FUTEXES 4096
#define LOOP 2
#define FUTEXES 2048
#define REPEAT_MEASUREMENT 512
#define AVERAGE (1<<3)
pthread_t tids[FUTEXES*LOOP];
unsigned int probe_futexes[TOTAL_FUTEXES];
volatile unsigned int futexes[FUTEXES*LOOP*4096];

static int futex(unsigned int *uaddr, int futex_op, unsigned int val, const struct timespec *timeout, unsigned int *uaddr2, unsigned int val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}
void *do_job(void *arg)
{
    int ret;
    size_t id = (size_t)arg;
    id = (id*4096) | (id*8 % 4096);
    ret = futex((unsigned int *)&futexes[id], FUTEX_WAIT_PRIVATE, 0, NULL, NULL, 0);
    if (ret < 0) {
        perror("futex do_job");
        exit(-1);
    }
    return 0;
}
int compare(const void *a, const void *b)
{
  return (*(size_t *)a - *(size_t *)b);
}

void print_usage(char *name)
{
    printf("%s <struct_agnostic_amp=0> <core=7> <file_name=futex_hash_table_%%s.csv>\n", name);
    exit(0);
}

char *name = 0;
size_t software_agnostic_amplification = 0;
size_t core = 7;
int main(int argc, char **argv)
{
    static volatile char flush_cache[(1<<10)*(1<<7)];
    if (argc > 4)
        print_usage(argv[0]);
    software_agnostic_amplification = (argc > 1) && strcmp(argv[1], "0");
    if (argc > 2)
        core = atoi(argv[2]);
    if (argc > 3)
        name = argv[3];
    int ret;
    size_t ground_truths[TOTAL_FUTEXES*LOOP] = {0};
    size_t times[TOTAL_FUTEXES*LOOP];
    pin_to_core(core);
    helper_init();
    printf("[*] warmup\n");
    for (volatile size_t i = 0; i < 0x400000000; ++i);
    printf("[*] start\n");
    for (size_t j = 0; j < LOOP; ++j) {
        printf("[*] %zd/%d\n", j, LOOP);
        for (size_t i = 0; i < FUTEXES; ++i) {
            size_t id = i+FUTEXES*j;
            ret = pthread_create(&tids[id], 0, do_job, (void *)id);
            if (ret < 0) {
                perror("pthread_create");
                exit(-1);
            }
        }
        for (size_t i = 0; i < TOTAL_FUTEXES; ++i) {
            size_t t0;
            size_t t1;
            size_t time = 0;
            size_t __times[REPEAT_MEASUREMENT];
            for (size_t l = 0; l < REPEAT_MEASUREMENT; ++l) {
                sched_yield();
                if (software_agnostic_amplification)
                    memset((char *)flush_cache, 1, sizeof(flush_cache));
                t0 = rdtsc_begin();
                ret = futex(&probe_futexes[i], FUTEX_WAKE_PRIVATE, 0x10, NULL, NULL, 0);
                t1 = rdtsc_end();
                __times[l] = t1 - t0;
                if (ret < 0) {
                    perror("futex main");
                    exit(-1);
                }
            }
            qsort(__times, REPEAT_MEASUREMENT, sizeof(size_t), compare);
            for (size_t l = 0; l < AVERAGE; ++l)
                time += __times[l];
            time /= AVERAGE;
            times[i+j*TOTAL_FUTEXES] = time;
            futex_read_count((size_t)&ground_truths[i+j*TOTAL_FUTEXES], (size_t)&probe_futexes[i]);
        }
        sleep(1);
    }
    for (size_t i = 0; i < TOTAL_FUTEXES*LOOP; ++i) {
        if (i % 16 == 0)
            printf("[%4zd] ", i);
        printf("%4zd (%zd)", times[i], ground_truths[i]);
        if (i % 16 == 15)
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
        snprintf(file_name, sizeof(file_name), "futex_hash_table_%s.csv", buffer);
        name = file_name;
    }
    FILE *csv_file = fopen(name, "w");

    if (csv_file == NULL) {
        printf("[!] fopen\n");
        return 1;
    }

    // Write headers
    fprintf(csv_file, "ground truth; time\n");

    for (size_t i = 0; i < TOTAL_FUTEXES*LOOP; ++i) {
        fprintf(csv_file, "%ld; %ld\n", ground_truths[i], times[i]);
    }

    fclose(csv_file);
    printf("[+] done\n");
}
