#pragma once

#define TIMER_READ_CNT_HASH 12
#define TIMER_READ_INDEX_HASH 13
#define KEY_READ_CNT 14
#define FUTEX_READ_CNT 15

typedef union {
    struct timer_rd {
        size_t timer_id;
        size_t uaddr;
    } timer_rd;
    struct key_rd {
        size_t key_id;
        size_t uaddr;
    } key_rd;
    struct futex_rd {
        size_t futex_addr;
        size_t uaddr;
    } futex_rd;
} msg_t;