#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // for uintN_t
#include <time.h> // for sync of sender and receiver
#include <malloc.h> // for realloc

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
    // write interval input
    uint64_t interval = atoi(argv[2]);
    // set up variables for the input string
    char input [1500];
    uint16_t true_length;
    uint16_t payload_length;
    uint8_t* ethernet_frame = NULL;
    
    // set up variables for timing during sending
    struct timespec time;
    struct timespec time2;
    uint64_t end_nsec;
    uint64_t end_sec;
    uint64_t store; // used to cache function

    start: // jump to start to send new Ethernet frame

    // read input payload
    printf("Message:\n");
    strcpy(input, "Dies ist der Teststring, der mehrfach übertragen wird!\n");
    printf("%s",input);

    //fgets(input, 1500, stdin);
    true_length = strlen(input) - 1; // length of string - \n
    payload_length; // length that has to be padded up to 46 bytes
    // when the input is less than 46 bytes, set payload_length to 64 and pad with 0
    if(true_length<46)
    {
        payload_length = 46;
    }
    else
    {
        payload_length = true_length;
    }
    // generate ethernet frame
    // preamble + mac header + payload + checksum
    if((ethernet_frame = realloc(ethernet_frame, sizeof(uint8_t)*(64 + 112 + payload_length*8 + 32)))
        == NULL)
        {
            printf("Couldn't allocate memory\n");
            return 1;
        }
    // set all bits to 0
    memset(ethernet_frame, 0, 64 + 112 + payload_length*8 + 32);
    map_ethernet_frame(ethernet_frame, true_length, input);
    // memset(ethernet_frame, 0, 10000000);
    // map_alternatingbits(ethernet_frame);
    // write ethernetframe bits to txt (compare with recv txt)
    FILE* fp_exec;
    fp_exec = fopen("sen_exec.txt", "w" );
    for (int i = 0; i < (112 + payload_length*8 + 32); i++)
    {
        fprintf(fp_exec,"%i", ethernet_frame[i+64]);
    }
    fprintf(fp_exec, "2"); // as end of ethernet frame
    fflush(fp_exec);
    
    clock_gettime(CLOCK_MONOTONIC, &time);
    end_nsec = time.tv_nsec;
    end_sec = time.tv_sec;
    for(int i = 0; i < (64 + 112 + payload_length*8 + 32); i++)
    {
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

                store = *((uint64_t*)&nop);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }
            else if(ethernet_frame[i]==1&&mode==0) // bit 1 and F+F
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                store = *((uint64_t*)&nop);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }
            else if(!(ethernet_frame[i]|mode))     // bit 0 and F+F
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                flush_cache_line(nop);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }
            else                            // bit 0 and F+R
            {
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");

                flush_cache_line(nop);
                                
                // data, instruction barrier
                asm volatile ("DSB SY");
                asm volatile ("ISB");
            }


            // check if frame is over
            clock_gettime(CLOCK_MONOTONIC, &time);
            if(time.tv_nsec>=end_nsec&&time.tv_sec>=end_sec)
            {
                clock_gettime(CLOCK_MONOTONIC, &time2);
                // printf("%i(%i):%i\n",i, ethernet_frame[i] ,time2.tv_nsec);
                break;
            }


        }
    }
    // preparing to send new ethernet frame
    printf("Preparing to send next Ethernet frame\n");
    printf("----------------------------------\n");
    sleep(1);
    goto start; // jump to start to send new Ethernet frame
    return 0;
}