#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // for uintN_t
#include <time.h> // for sync of sender and receiver

// for PERF_COUNT_HW_CPU_CYCLES (timing)
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "sharedmem.h"



int main()
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    uint64_t start_nsec;
    uint64_t start_sec;
    uint64_t frequency = 500000;

    for(int i = 0; i< 100000; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &time);
        start_nsec = time.tv_nsec;
        start_sec = time.tv_sec;

        // data, instruction barrier
        asm volatile ("DSB SY");
        asm volatile ("ISB");

        asm volatile ("MOV X0, %0;"
                        :: "r"  (buffer[10]));
        // data, instruction barrier
        asm volatile ("DSB SY");
        asm volatile ("ISB");

        start_nsec += frequency;
        if(start_nsec > 999999999) // nanoseconds overflow
        {
            start_nsec -= 1000000000;
            start_sec++;
        }
        time.tv_nsec = start_nsec;
        time.tv_sec = start_sec;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);
    }
    return 0;
}