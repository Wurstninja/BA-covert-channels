#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // for uintN_t

// for PERF_COUNT_HW_CPU_CYCLES (timing)
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "sharedmem.h"

int main()
{
    for(int i = 0; i< 10000; i++)
    {

        // data, instruction barrier
        asm volatile ("DSB SY");
        asm volatile ("ISB");

        asm volatile ("MOV X0, %0;"
                        :: "r"  (buffer[10]));
        // data, instruction barrier
        asm volatile ("DSB SY");
        asm volatile ("ISB");
    }
    return 0;
}