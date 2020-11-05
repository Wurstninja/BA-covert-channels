void flush_cache_line(void* addr)
{
    // clean and invalidate cacheline by VA to PoC
    asm volatile (" DC CIVAC, %0;"
                    : // no output
                    : "r" (addr));
    asm volatile ("DSB ISH"); // data sync barrier to inner sharable domain
    asm volatile ("ISB"); //instruction sync barrier
}