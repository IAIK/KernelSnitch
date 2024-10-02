#include "cacheutils.h"
#include "helper.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define KEY_MEASURE 0xbadc0fe
#define KEY_START 0xdeadbeef
#define KEY_WARMUP 0xbadbabe

#define REPEAT_MEASUREMENT 1024
#define AVERAGE (1<<3)
#define OFFSET 2
#define KEYS 8*1024

#define TRIES 8

#define WARMUP_KEYS 4096
size_t msgiqs[KEYS];

int compare(const void *a, const void *b)
{
  return (*(size_t *)a - *(size_t *)b);
}

void cleanup(void)
{
    printf("[*] cleanup\n");
    for (size_t i = 0; i < KEYS; ++i) {
        msgctl(msgiqs[i], IPC_RMID, NULL);
    }
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

    helper_init();
    pin_to_core(core);

    printf("[*] warmup\n");
    for (volatile size_t i = 0; i < 0x400000000; ++i);

    int ret;
    size_t times[KEYS];
    size_t times_std[KEYS];
    size_t ground_truth[KEYS];
    // size_t warmup_msgiqs[WARMUP_KEYS];
    // printf("[*] allocate warmup keys\n");
    // for (size_t i = 0; i < WARMUP_KEYS; ++i) {
    //     ret = msgget(KEY_WARMUP+i, IPC_CREAT);
    //     if (ret < 0) {
    //         printf("[!] i %ld\n", i);
    //         perror("msgget(KEY_WARMUP+i, IPC_CREAT)");
    //         exit(-1);
    //     }
    //     warmup_msgiqs[i] = ret;
    // }

    printf("[*] start measure\n");
    for (size_t i = 0; i < KEYS; ++i) {
        if (i % 16 == 0)
            printf("[*] %ld/%d\n", i, KEYS);

        ret = msgget(KEY_START+i, IPC_CREAT);
        if (ret < 0) {
            printf("[!] i %ld\n", i);
            perror("msgget(KEY_START+i, IPC_CREAT)");
            cleanup();
            exit(-1);
        }
        msgiqs[i] = ret;

        size_t tries = TRIES;
redo:
        if (tries-- == 0) {
            printf("[!] no more tries\n");
            cleanup();
            exit(-1);
        }
        size_t t0;
        size_t t1;
        size_t time = 0;
        size_t measured_times[REPEAT_MEASUREMENT];
        for (size_t j = 0; j < REPEAT_MEASUREMENT; ++j) {
            sched_yield();
            if (software_agnostic_amplification)
                memset((char *)flush_cache, 1, sizeof(flush_cache));
            t0 = rdtsc_begin();
            ret = msgget(KEY_MEASURE+i, 0);
            // ret = msgget(KEY_START+i, IPC_CREAT | IPC_EXCL);
            t1 = rdtsc_end();
            measured_times[j] = t1 - t0;
            if (ret != -1) {
                printf("[!] i %ld ret %d\n", i, ret);
                perror("msgget(KEY_MEASURE+i, 0)");
                // perror("msgget(KEY_START+i, IPC_CREAT | IPC_EXCL)");
                cleanup();
                exit(-1);
            }
        }
        qsort(measured_times, REPEAT_MEASUREMENT, sizeof(size_t), compare);
        for (size_t i = 0; i < AVERAGE; ++i)
            time += measured_times[OFFSET+i];
        time /= AVERAGE;
        times[i] = time;
        times_std[i] = measured_times[AVERAGE-1+OFFSET] - measured_times[OFFSET];
        if (times_std[i] > 20/*2*/) {
            printf("[*] %ld/%d\n", tries, TRIES);
            goto redo;
        }
        ret = key_read_index((size_t)&ground_truth[i], KEY_MEASURE+i);
        if (ret < 0) {
            printf("[!] key_read_index error %d\n", ret);
            cleanup();
            exit(-1);
        }
    }

    printf("[*] print results\n");
    for (size_t i = 0; i < KEYS; ++i) {
        if (i % 16 == 0)
            printf("[%4zd] ", i);
        // printf("%4zd(%3zd) [%zd]", times[i], times_std[i], ground_truth[i]);
        printf("%4zd [%zd]", times[i], ground_truth[i]);
        if (i % 16 == 15)
            printf("\n");
    }

    cleanup();

    if (name == 0) {
        printf("[?] save to file? [y/N] ");
        char in = getchar();
        if (in != 'y')
            exit(0);
    }

    time_t current_time;
    struct tm *timeinfo;
    char buffer[80];

    time(&current_time);
    timeinfo = localtime(&current_time);
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);

    if (name == 0) {
        static char file_name[0x1000];
        snprintf(file_name, sizeof(file_name), "ipc_ids_key_ht_%s.csv", buffer);
        name = file_name;
    }
    FILE *csv_file = fopen(name, "w");

    if (csv_file == NULL) {
        printf("[!] fopen\n");
        return 1;
    }

    // Write headers
    fprintf(csv_file, "ground truth; time\n");

    for (size_t i = 0; i < KEYS; ++i) {
        fprintf(csv_file, "%ld; %ld\n", ground_truth[i], times[i]);
    }

    fclose(csv_file);
    printf("[+] done\n");

    return 0;
}
