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
#include "flush.h"
#include "ethernet_setup.h"



int main(int argc, char* argv[])
{
    uint8_t mode = 42;
    // check if F+F or F+R
    if(strcmp(argv[1],"FF")==0)
    {
        mode = 0;
    }
    if(strcmp(argv[1],"FR")==0)
    {
        mode = 1;
    }
    if(mode==42) // if mode is neither FF nor FR
    {
        printf("Error: Unknown Mode\n");
        printf("ARGS should look like: <mode>\n");
        printf("with <mode>: FF, FR\n");
        exit(1);
    }

    uint16_t inputlength = 250; // set to inputlength (check if more than 36 and less than 1500)
    // generate ethernet frame

    uint8_t preamble [56];
    uint8_t sfd [8];
    uint8_t macheader [112];
    uint8_t payload [inputlength];
    uint8_t checksum [32];

    uint8_t ethernet_frame[2000];

    set_preamble(preamble);
    set_sfd(sfd);
    set_macheader(macheader, inputlength);
    // set payload
    // set checksum
    map_ethernet_frame(ethernet_frame);
    

    struct timespec time;
    uint64_t end_nsec;
    uint64_t end_sec;
    uint64_t interval = 10000000;
    uint64_t store; // used to cache puts 
    for(int i = 0; i < 100; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &time);
        end_nsec = time.tv_nsec;
        end_sec = time.tv_sec;

        end_nsec += interval;
            if(end_nsec > 999999999) // nanoseconds overflow
            {
                end_nsec -= 1000000000;
                end_sec++;
            }
        while(1) // do while still in the same frame
        {
            if(ethernet_frame[i]&mode)             // bit 1 and F+R
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                store = *((uint64_t*)&puts);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }
            else if(ethernet_frame[i]==1&&mode==0) // bit 1 and F+F
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                store = *((uint64_t*)&puts);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }
            else if(!(ethernet_frame[i]|mode))     // bit 0 and F+F
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                flush_cache_line(puts);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }
            else                            // bit 0 and F+R
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                flush_cache_line(puts);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }

            
            // check if frame is over
            clock_gettime(CLOCK_MONOTONIC, &time);
            if(time.tv_nsec>=end_nsec&&time.tv_sec>=end_sec)
            {
                break;
            }


        }
    }
    return 0;
}