#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // for uintN_t

// for PERF_COUNT_HW_CPU_CYCLES (timing)
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include <sys/types.h>

#include "flush.h"
#include "sharedmem.h"
#include "calibrate.h"

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                    group_fd, flags);
    return ret;
}


int main()
{   
    // setting up PERF_COUNT_HW_CPU_CYCLES
    struct perf_event_attr pe;
    uint64_t count;
    int fd;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
        fprintf(stderr, "Error opening leader %llx\n", pe.config);
        exit(EXIT_FAILURE);
    }

    uint64_t ffthreshold = flush_flush_threshold(addr, pe, count, fd);

    printf("Recommended Threshold: %lli\n");

    if(fork() == 0)
    {
        printf("Hi, from child\n");
        sleep(1);
        for(int i = 0; i< 100000; i++)
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
    }
    else
    {
        uint64_t sum = 0;
        uint64_t hits = 0;
    for(int i = 0; i<10000000; i++)
    {
        asm volatile ("DSB SY");
        asm volatile ("ISB");
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        flush_cache_line(addr);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));

        sum +=count;

        if(count>ffthreshold)
        {
            hits++;
        }
        if(i%10000==0)
        {
            sum = sum /10000;
            printf("AVG: %lli", sum);
            sum=0;

            printf(", Hits:%lli\n", hits);
            hits = 0;
        }
    }
    
        
    }
    

    //printf("%lli,%lli\n",count,count2);

    return 0;
}