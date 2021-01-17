#include <stdlib.h>
#include <stdio.h>
#include <stdint.h> // for uintN_t
#include <inttypes.h> // to print uintN_t
#include <time.h> // for sync of sender and receiver
#include <sys/wait.h> // letting parent wait for child

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
        threshold = flush_flush_threshold(nop, pe, count, fd);
    }
    if(mode) // FR
    {
        threshold = flush_reload_threshold(nop, pe, count, fd);
    }

    // open python script to calc threshold
    uint32_t statval;
    if(fork() == 0)
    {
        printf("Calculating threshold ... \n");
        if(mode) // depending on mode selected start F+R or F+F plot
        {
            execlp("python", "python", "frplot.py", NULL, (char*) NULL);
        }
        else
        {
            execlp("python", "python", "ffplot.py", NULL, (char*) NULL);
        }
        
    }
    else
    {
        wait(&statval);
        if(!WIFEXITED(statval))
        {
            printf("Child did not terminate with exit\n");
        }
    }

    printf("threshold: ");
    scanf("%li", &threshold);


    // set up timing sync
    struct timespec time;
    struct timespec time2; // time2 used for debugging
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
    uint8_t maxcorrect = 4; // correct false pos and neg by reseting when N same bits in a row
    uint8_t correct = maxcorrect; // decrement for every same bit in a row

    // detect sfd
    uint8_t sfd [8];
    set_sfd(sfd);
    uint8_t sfd_counter = 0;
    uint8_t sfd_maxcorrect = 1; // correct N wrong bits
    uint8_t sfd_correct = sfd_maxcorrect; // decrement for every wrong bit

    // ethernet frame
    uint8_t ethernet_frame [14+1500] = {0}; // store bytes for mac header + payload (used later for crc)
    uint16_t ethernet_frame_counter = 0;
    // also used for payload
    uint8_t bit_counter = 0;        // count bits to know when full char is read
    uint8_t cur_char = 0;           // used to shift received bits onto

    // read mac header
    uint8_t macheader_counter = 0;
    uint16_t ethertype = 0;     // stores payloadlength
    uint16_t ethertype_counter = 0;

    // read payload
    
    uint16_t char_counter = 0;
    uint8_t output [1500] = {0};

    // read crc
    uint8_t crc_counter = 0;
    uint32_t crc = 0;             // using 16 bit crc, but still sending 32 bit (16 bit padded with 0)
    char* buffer = malloc(sizeof(char)); // store bytes on buffer and then check with crc

    FILE* fp_exec;
    fp_exec = fopen("rec_exec.txt", "w" );
    // gettime as reference for when the frame is over
    clock_gettime(CLOCK_MONOTONIC, &time);

    while(1) // receive loop
    {
        //clock_gettime(CLOCK_MONOTONIC, &time2);
        //printf("%i\n",time2.tv_nsec); // just for debugging
        if(mode) // F+R
        {
            // data, instruction barrier
            asm volatile ("DSB SY");
            asm volatile ("ISB");
            ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

            store = *((uint64_t*)&nop);

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
            // print bits to txt when ethernet frame started (when sfd is finished)
            if(sfd_counter == 8)
            {
                fprintf(fp_exec,"%i",hit);
            }
        }
        else // F+F
        {
            // data, instruction barrier
            asm volatile ("DSB SY");
            asm volatile ("ISB");
            ioctl(fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

            flush_cache_line(nop);

            // data, instruction barrier
            asm volatile ("DSB SY");
            asm volatile ("ISB");

            ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
            read(fd, &count, sizeof(long long));
            if(preamble_counter<56)
            {
                printf("%lli\n",count);
            }
            if(count<threshold)
            {
                hit = 0;
            }
            else
            {
                hit = 1;
            }
        }
        /*if(!hit)
        {
            printf("miss %lli\n", count);
        }
        else
        {
            printf("hit %lli\n", count);
        }*/

        // detect following preamble bits 
        if(preamble_counter<56)
        {
            // printf("pre:%i\n",preamble_counter); // just for debugging
            // detect first preamble bit (stricter correct than following bits)
            // F+F by default detects misses, so a hit should indicate preamble start
            if(preamble_counter == 0 && !mode) // mode = 0 -> F+F
            {
                if(hit)
                {
                    correct = 0;
                    last_bit = hit;
                    preamble_counter = 1;
                    goto end;
                }
                else
                {
                    goto end;
                }
            }
            // F+R by default detects hits, so a miss should indicate second preamble bit
            if(preamble_counter == 0 && mode) // mode = 1 -> F+R
            {
                if(hit)
                {
                    goto end;
                }
                else
                {   
                    correct = 0;
                    last_bit = hit;
                    preamble_counter = 2;   // for unknown reason first miss is never detected
                    goto end;               // there set preamble_counter to 4 instead of 2
                }
            }
            // detect following preamble bits
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
            end:
            asm("nop");
        }
        else if(sfd_counter<8)
        {
            // printf("sfd:%i\n",sfd_counter); // just for debugging
            if((hit)!=sfd[sfd_counter]) // when received bit doesn't match with sfd bit
            {
                sfd_correct--;
            }
            
            // in last step
            if(sfd_counter==7&&sfd_correct<0)
            {
                // if sfd bits don't match, reset everything
                printf("SFD bits don't match\n");
                preamble_counter = 0;
                sfd_counter = 0;
            }
            if(sfd_counter==7&&sfd_correct>0)
            {
                printf("SFD bits matched\n");
            }
            sfd_counter++;
        }
        else if(macheader_counter<112)
        {
            // printf("mac:%i\n",macheader_counter); // just for debugging
            //shift bit onto cur_char
            cur_char = cur_char | (hit)<<(7-bit_counter);
            // in last bit of cur_char step
            if(bit_counter == 7)
            {
                ethernet_frame[ethernet_frame_counter] = cur_char;
                // next char
                bit_counter = 0;
                ethernet_frame_counter++;
                cur_char = 0;
            }
            else
            {
                bit_counter++;
            }
            if(macheader_counter>95)
            {
                ethertype = ethertype | (hit)<<(15-ethertype_counter);
                ethertype_counter++;
                // in last step print payloadlength
                if(macheader_counter==111)
                {
                    printf("Payloadlength:%i\n",ethertype);
                    if(ethertype<46)
                    {
                        ethertype = 46;
                    }
                    if(ethertype > 1500)
                    {
                        printf("Payloadlength must be below 1500 characters\n");
                        ethertype = 1500;
                    }
                    ethertype = 54;
                }
            }   
            macheader_counter++; 
        }
        else if(char_counter<ethertype+1)
        {
            //shift bit onto cur_char
            cur_char = cur_char | (hit)<<(7-bit_counter);
            // in last bit of cur_char step
            if(bit_counter == 7)
            {
                output[char_counter] = cur_char;
                ethernet_frame[ethernet_frame_counter] = cur_char;
                // printf("-----------------%c++%i\n",(char)cur_char,cur_char);
                printf("%c",(char)cur_char);
                fflush(stdout);
                // store to output
                // next char
                bit_counter = 0;
                char_counter++;
                ethernet_frame_counter++;
                cur_char = 0;
            }
            else
            {
                bit_counter++;
            }
        }
        else if(crc_counter<32)
        {
            crc = crc | (hit)<<(31-crc_counter);
            // in last step
            if(crc_counter == 31)
            {
                buffer = realloc(buffer, sizeof(char)*(ethertype+14));
                // map macheader and payload onto buffer
                for(int i = 0; i < ethertype; i++)
                {
                    buffer[0] = (char)ethernet_frame [0];
                }
                // if crc matches
                if(crc == (uint32_t)gen_crc16(buffer,14+ethertype))
                {
                    printf("\nCRC matched!\n");
                }
                else
                {
                    printf("\nCRC doesn't match\n");
                }
                fprintf(fp_exec,"2");
                // fflush
            }
            crc_counter++;
        }
        else
        {
            printf("\nPreparing for new Ethernet frame\n");
            printf("----------------------------------\n");

            // flush pipeline
            fflush(fp_exec);
            
            // reset counter and variables 
            preamble_counter = 0;
            correct = maxcorrect;
            sfd_counter = 0;
            sfd_correct = sfd_maxcorrect;
            macheader_counter = 0;
            char_counter = 0;
            ethertype = 0;
            ethertype_counter = 0;
            crc_counter = 0;
            crc = 0;
            ethernet_frame_counter = 0;
            
        }

        // print bits to txt when ethernet frame started (when sfd is finished)
            if(sfd_counter == 8)
            {
                fprintf(fp_exec,"%i",hit);
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