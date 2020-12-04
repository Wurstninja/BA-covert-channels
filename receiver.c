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
    // unlink shared memory
    shm_unlink("sharedmem");

    // create shared mem
    int shm_fd = shm_open("sharedmem", O_CREAT | O_EXCL | O_RDWR,
                                 S_IRUSR | S_IWUSR);
        if (shm_fd == -1)
        {
            printf("shm_open failed\n");
            exit(1);
        }
    // set size of shared mem object              
    if (ftruncate(shm_fd, SIZE) == -1)
    {
        printf("ftruncate failed\n");
        exit(1);
    }
    // map shared mem into memory
    uint64_t* sharedmem = (uint64_t*) mmap(NULL, SIZE, PROT_READ | PROT_WRITE, 
                    MAP_SHARED, shm_fd, 0);
        if (sharedmem == MAP_FAILED)
        {
            printf("mmap failed\n");
            exit(1);
        }
    uint64_t* addr = (sharedmem+10);
    
    
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

    // calc threshold
    uint64_t threshold = flush_flush_threshold(addr, pe, count, fd);

    printf("Recommended Threshold: %lli\n", threshold);

    // set up timing sync

    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    uint64_t start_nsec;
    uint64_t start_sec;
    uint64_t interval = 100000000;
    uint64_t hits = 0;
    uint64_t miss = 0;

    FILE* fp_exec;
    fp_exec = fopen("ffexec.txt", "w" );

    for(int i = 0; i<1000; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &time);
        start_nsec = time.tv_nsec;
        start_sec = time.tv_sec;


        // data, instruction barrier
        asm volatile ("DSB SY");
        asm volatile ("ISB");
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        flush_cache_line(puts);

        // data, instruction barrier
        asm volatile ("DSB SY");
        asm volatile ("ISB");

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));

        fprintf(fp_exec,"%lli\n", count);
        /*if(count<=threshold)
        {
            printf("miss %lli\n", count);
        }
        else
        {
            printf("hit %lli\n", count);
            hits++;
        }*/
        if(i%100==0)
        {
            printf("%lli \n", hits);
            hits = 0;
        }
        if(i%1000==0) 
        {
            printf("\n");
        }
        
        start_nsec += interval;
        if(start_nsec > 999999999) // nanoseconds overflow
        {
            start_nsec -= 1000000000;
            start_sec++;
        }
        time.tv_nsec = start_nsec;
        time.tv_sec = start_sec;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);
    }

    // unlink shared memory
    shm_fd = shm_unlink("sharedmem");
        if (shm_fd == -1)
        {
            printf("shm_unlink failed\n");
            exit(1);
        }

    return 0;
}