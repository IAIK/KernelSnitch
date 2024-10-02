#include "cacheutils.h"
#include <stdio.h>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/resource.h>

#define REPEAT_MEASUREMENT 1024
#define AVERAGE (1<<3)
#define OFFSET 2
#define TIMERS 1024
#define TRIES 8

int fd_timer_list;
size_t get_ground_truth(size_t cpu)
{
    static char buffer[1<<30];
    char *running = buffer;
    size_t size = sizeof(buffer);
    int ret;
    lseek(fd_timer_list, 0, SEEK_SET);
    for (size_t i = 0; i < 10; ++i) {
        ret = read(fd_timer_list, running, size);
        if (ret < 0)
            break;
        running += ret;
        size -= ret;
    }

    char start[100];
    snprintf(start, sizeof(start), "cpu: %ld", cpu);

    char *str = strstr(buffer, start);
    if (str == 0) {
        printf("[!] strstr(buffer, \"cpu: CPU\")\n");
        exit(-1);
    }
    str = strstr(str, "active timers:");
    if (str == 0) {
        printf("[!] strstr(buffer, \"active timers\")\n");
        exit(-1);
    }

    char *res = 0;
    char *pch = strtok(str, "\n");
    while (pch != NULL) {
        if (strstr(pch, "clock 1:") != 0)
            break;
        char *tmp = strstr(pch, "expires");
        if (tmp == 0)
            res = pch;
        // printf("%s\n", pch);
        pch = strtok(NULL, "\n");
    }
    res++;
    res++;
    char *tmp = res;
    while (1) {
        if (*tmp == ':') {
            *tmp = '\0';
            break;
        }
        tmp++;
    }
    // printf(res);
    return atoi(res);
}

int compare(const void *a, const void *b)
{
    return (*(size_t *)a - *(size_t *)b);
}

void print_usage(char *name)
{
    printf("%s <struct_agnostic_amp=0> <core=7> <file_name=hrtimer_bases_clock_base_active_%%s.csv>\n", name);
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
    struct rlimit l = {
        .rlim_cur = 131072,
        .rlim_max = 131072,
    };
    ret = setrlimit(RLIMIT_NOFILE, &l);
    if (ret < 0) {
        perror("setrlimit");
        exit(-1);
    }

    fd_timer_list = open("/proc/timer_list", O_RDONLY);
    if (fd_timer_list < 0) {
        perror("open(/proc/timer_list)");
        exit(-1);
    }

    pin_to_core(core);
    printf("[*] warmup\n");
    for (volatile size_t i = 0; i < 0x4000000; ++i);

    size_t times[TIMERS];
    size_t times_std[TIMERS];
    size_t ground_truth[TIMERS];

    int fd_probe = timerfd_create(CLOCK_REALTIME, 0);
    if (fd_probe < 0) {
        perror("timerfd_create");
        exit(-1);
    }

    struct itimerspec spec = {
        { 1000000, 1000000 },
        { 1000000, 1000000 }
    };

    printf("[*] start measure\n");
    for (size_t i = 0; i < TIMERS; ++i) {
        if (i % 16 == 0)
            printf("[*] %ld/%d\n", i, TIMERS);

        int fd = timerfd_create(CLOCK_REALTIME, 0);
        if (fd < 0) {
            perror("timerfd_create");
            exit(-1);
        }
        int ret = timerfd_settime(fd, 0, &spec, NULL);
        if (ret < 0) {
            perror("timerfd_settime");
            exit(-1);
        }

        size_t tries = TRIES;
redo:
        if (tries-- == 0) {
            printf("[!] no more tries\n");
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
            int ret = timerfd_settime(fd_probe, 0, &spec, NULL);
            t1 = rdtsc_end();
            measured_times[j] = t1 - t0;
            if (ret < 0) {
                printf("[!] i %ld ret %d\n", i, ret);
                perror("timerfd_settime");
                exit(-1);
            }
        }
        qsort(measured_times, REPEAT_MEASUREMENT, sizeof(size_t), compare);
        for (size_t i = 0; i < AVERAGE; ++i)
            time += measured_times[OFFSET+i];
        time /= AVERAGE;
        times[i] = time;
        times_std[i] = measured_times[AVERAGE-1+OFFSET] - measured_times[OFFSET];
        if (times_std[i] > 5) {
            printf("[*] %ld/%d\n", tries, TRIES);
            goto redo;
        }
        ground_truth[i] = get_ground_truth(core);
    }

    printf("[*] print results\n");
    for (size_t i = 0; i < TIMERS; ++i) {
        if (i % 16 == 0)
            printf("[%4zd] ", i);
        printf("%4zd [%4zd]", times[i], ground_truth[i]);
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
    char buffer[80];

    time(&current_time);
    timeinfo = localtime(&current_time);
    strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", timeinfo);

    if (name == 0) {
        static char file_name[0x1000];
        snprintf(file_name, sizeof(file_name), "hrtimer_bases_clock_base_active_%s.csv", buffer);
        name = file_name;
    }
    FILE *csv_file = fopen(name, "w");

    if (csv_file == NULL) {
        printf("[!] fopen\n");
        return 1;
    }

    // Write headers
    fprintf(csv_file, "ground truth; time\n");

    for (size_t i = 0; i < TIMERS; ++i) {
        fprintf(csv_file, "%ld; %ld\n", ground_truth[i], times[i]);
    }

    fclose(csv_file);
    printf("[+] done\n");

    return 0;
}
