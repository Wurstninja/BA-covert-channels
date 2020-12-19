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
#include <string.h>

#include "flush.h"
#include "sharedmem.h"
#include "calibrate.h"
#include "ethernet_setup.h"

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                            int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                    group_fd, flags);
    return ret;
}


int main(int argc, char* argv[])
{   
    uint8_t mode = 42;
    // check if F+F or F+R
    if(strcmp(argv[1],"FF")==0)
    {
        mode = 0; 
        printf("FF");
    }
    if(strcmp(argv[1],"FR")==0)
    {
        mode = 1;
        printf("FR");
    }
    if(mode==42) // if mode is neither FF nor FR
    {
        printf("Error: Unknown Mode\n");
        printf("ARGS should look like: <mode>\n");
        printf("with <mode>: FF, FR\n");
        exit(1);
    }
    
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

    // calc threshold depending on mode (FF=0,FR=1)
    uint64_t threshold;
    if(!mode) // FF
    {
        threshold = flush_flush_threshold(puts, pe, count, fd);
    }
    if(mode) // FR
    {
        threshold = flush_reload_threshold(puts, pe, count, fd);
    }

    printf("Recommended threshold is: %lli\n",threshold);
    printf("threshold: ");
    scanf("%li", &threshold);


    // set up timing sync
    struct timespec time;
    struct timespec time2;
    uint64_t interval = 10000000;
    uint64_t store; // used to cache data during F+R

    // boolean to know if current bit is hit or miss
    uint8_t hit;

    // for debugging
    uint64_t hits = 0;
    uint64_t miss = 0;

    // detect preamble
    uint8_t preamble_counter = 0;
    uint8_t last_bit = 0;
    uint8_t maxcorrect = 5; // correct false pos and neg by reseting when N same bits in a row
    uint8_t correct = maxcorrect; // decrement for every same bit in a row

    // detect sfd
    uint8_t sfd [8];
    set_sfd(sfd);
    uint8_t sfd_counter = 0;
    uint8_t sfd_maxcorrect = 2; // correct N wrong bits
    uint8_t sfd_correct = sfd_maxcorrect; // decrement for every wrong bit

    // read mac header
    uint8_t macheader_counter;
    uint16_t ethertype = 0;
    uint16_t ethertype_counter = 0;

    FILE* fp_exec;
    fp_exec = fopen("ffexec.txt", "w" );
    // gettime as reference for when the frame is over
    clock_gettime(CLOCK_MONOTONIC, &time);

    while(1) // receive loop
    {
        if(mode) // F+R
        {
            // data, instruction barrier
            asm volatile ("DSB SY");
            asm volatile ("ISB");
            ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

            store = *((uint64_t*)&puts);

            // data, instruction barrier
            asm volatile ("DSB SY");
            asm volatile ("ISB");

            ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
            read(fd, &count, sizeof(long long));
            if(count>threshold)
            {
                hit = 0;
            }
            else
            {
                hit = 1;
            }
        }
        else // F+F
        {
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

            if(count<threshold)
            {
                hit = 0;
            }
            else
            {
                hit = 1;
            }
        }
    	// print timings (debug)
        fprintf(fp_exec,"%lli\n", count);
        if(!hit)
        {
            printf("miss %lli\n", count);
        }
        else
        {
            printf("hit %lli\n", count);
        }

        // detect preamble bits 
        if(preamble_counter<56)
        {
            if(hit) // hit
            {
                if(!last_bit)   // if last bit 0
                {
                    preamble_counter++;
                    last_bit = 1;
                    correct = maxcorrect;
                }
                else            // if last bit 1
                {
                    if(!correct)// if more than N false pos, neg in a row
                    {
                        preamble_counter = 0;
                        last_bit = 1;
                        correct = maxcorrect;
                    }
                    else        // if still below N false pos, neg in a row
                    {
                        preamble_counter++;
                        last_bit = 1;
                        correct--;
                    }

                }
            }
            else                // miss
            {
                if(last_bit)   // if last bit 1
                {
                    preamble_counter++;
                    last_bit = 0;
                    correct = maxcorrect;
                }
                else            // if last bit 0
                {
                    if(!correct)// if more than N false pos, neg in a row
                    {
                        preamble_counter = 0;
                        last_bit = 0;
                        correct = maxcorrect;
                    }
                    else        // if still below N false pos, neg in a row
                    {
                        preamble_counter++;
                        last_bit = 0;
                        correct--;
                    }

                }
            }
        }
        else if(sfd_counter<8)
        {
            if((hit)!=sfd[sfd_counter]) // when received bit doesn't match with sfd bit
            {
                sfd_correct--;
            }
            
            // in last step
            if(sfd_counter==7&&sfd_correct<=0)
            {
                // if sfd bits don't match, reset everything
                printf("SFD bits don't match\n");
                preamble_counter = 0;
                sfd_counter = 0;
            }
            if(sfd_counter==7&&sfd_correct>0)
            {
                printf("matchhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\n");
            }
            sfd_counter++;
        }
        else if(macheader_counter<112)
        {
            clock_gettime(CLOCK_MONOTONIC, &time2);
            printf("%lli",time2.tv_nsec);
            // ignore first 96 bits (dst, src address)
            if(macheader_counter>95)
            {
                ethertype = ethertype | (hit)<<(15-ethertype_counter);
                ethertype_counter++;
                printf("ethertype:%i\n",ethertype);
            }   
            macheader_counter++; 
        }

        
        time.tv_nsec += interval;
        if(time.tv_nsec > 999999999) // nanoseconds overflow
        {
            time.tv_nsec -= 1000000000;
            time.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &time, NULL);
    }
    return 0;
}