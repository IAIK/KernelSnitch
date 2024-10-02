#pragma once
#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "modules/helper.h"

static int fd;

int timer_read_count_hash(size_t uaddr, size_t timer_id)
{
    msg_t msg = {
        .timer_rd = {
            .timer_id = timer_id,
            .uaddr = uaddr,
        }
    };
	return ioctl(fd, TIMER_READ_CNT_HASH, (unsigned long)&msg);
}

int timer_read_index_hash(size_t uaddr, size_t timer_id)
{
    msg_t msg = {
        .timer_rd = {
            .timer_id = timer_id,
            .uaddr = uaddr,
        }
    };
	return ioctl(fd, TIMER_READ_INDEX_HASH, (unsigned long)&msg);
}

int key_read_index(size_t uaddr, size_t key_id)
{
    msg_t msg = {
        .key_rd = {
            .key_id = key_id,
            .uaddr = uaddr,
        }
    };
    return ioctl(fd, KEY_READ_CNT, (unsigned long)&msg);
}

int futex_read_count(size_t uaddr, size_t futex_addr)
{
    msg_t msg = {
        .futex_rd = {
            .futex_addr = futex_addr,
            .uaddr = uaddr,
        }
    };
    return ioctl(fd, FUTEX_READ_CNT, (unsigned long)&msg);
}

void helper_init(void)
{
	fd = open("/dev/helper", O_RDWR);
	if (fd < 0) {
		perror("open: ");
        exit(-1);
    }
}