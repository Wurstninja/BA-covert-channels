#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // for uintN_t

// for PERF_COUNT_HW_CPU_CYCLES (timing)
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

uint64_t buffer [24]; // 64*3 Byte (to ensure entire cache line length 64 Byte is occupied)
void* addr = ((void*)buffer)+64+16;

static long
       perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                       int cpu, int group_fd, unsigned long flags)
       {
           int ret;

           ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                          group_fd, flags);
           return ret;
       }

void flush_cache_line(void* addr)
{
    // clean and invalidate cacheline by VA to PoC
    asm volatile (" DC CIVAC, %0;"
                    : // no output
                    : "r" (addr));
    asm volatile ("DSB ISH"); // data sync barrier to inner sharable domain
    asm volatile ("ISB"); //instruction sync barrier
}

void data_access(int value)
{
    asm volatile ("MOV X0, %0;"
                    :: "r"  (value));
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");
}


int main()
{   
    // setting up PERF_COUNT_HW_CPU_CYCLES
    struct perf_event_attr pe;
    long long count;
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
    // first data access
    
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    printf("Used %lld instructions\n", count);

    

    // second data access
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    printf("Used %lld instructions\n", count);

    flush_cache_line(addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    // third data access

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    printf("Used %lld instructions\n", count);


    return 0;
}
