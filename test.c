#include <stdlib.h>
#include <stdio.h>
#include <time.h> // for clock_gettime
#include <stdint.h> // for uintN_t

struct timespec start, stop;
int x = 42;
int y = 0;
int output = 0;
uint64_t addr_x = (uint64_t)&x;

int main()
{
    // erster Zugriff
    clock_gettime(CLOCK_REALTIME, &start);

    asm volatile ("MOV %0, %1;"
                    : "=r" (output)
                    : "r"  (x));

    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");
    
    clock_gettime(CLOCK_REALTIME, &stop);
    

    printf("%li,%li\n", stop.tv_sec-start.tv_sec, stop.tv_nsec-start.tv_nsec);

    // zweiter Zugriff
    clock_gettime(CLOCK_REALTIME, &start);

    asm volatile ("MOV %0, %1;"
                    : "=r" (output)
                    : "r"  (x));

    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");
    
    clock_gettime(CLOCK_REALTIME, &stop);
    

    printf("%li,%li\n", stop.tv_sec-start.tv_sec, stop.tv_nsec-start.tv_nsec);

    // flush cacheline by VA to PoU
    asm volatile (" DC CIVAC, %0;"
                    : // no output
                    : "r" (addr_x));
    asm volatile ("DSB ISH"); // data sync barrier to inner sharable domain
    asm volatile ("ISB"); //instruction sync barrier

    // dritter Zugriff

    clock_gettime(CLOCK_REALTIME, &start);
    
    asm volatile ("MOV %0, %1;"
                    : "=r" (output)
                    : "r"  (x));

    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    clock_gettime(CLOCK_REALTIME, &stop);

    printf("%li,%li\n", stop.tv_sec-start.tv_sec, stop.tv_nsec-start.tv_nsec);
    return 0;
}