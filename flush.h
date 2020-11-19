void flush_cache_line(void* addr)
{
    // clean and invalidate cacheline by VA to PoC
    asm volatile (" DC CIVAC, %0;"
                    : // no output
                    : "r" (addr));
}

/*void flush_cache_line_I(void* addr)
{
    // clean and invalidate cacheline by VA to PoU
    asm volatile (" IC IALLU, %0;"
                    : // no output
                    : "r" (addr));
}*/