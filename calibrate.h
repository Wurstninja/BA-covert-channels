// compare function for q sort
int comp (const void* x, const void* y)
{
    uint64_t m = *((uint64_t*)x);
    uint64_t n = *((uint64_t*)y);
    if(m > n) return 1;
    if(m < n) return -1;
    return 0;
}

void flush_flush_timing(void* addr, struct perf_event_attr pe, uint64_t count, int fd, uint64_t* fftimings)
{
    // first flush
    flush_cache_line(addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    // not caching data

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // second flush
    flush_cache_line(addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    fftimings[0] = count;

    // printf("not cached: Used %lld instructions\n", count);

    // first flush
    flush_cache_line(addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    // cache data

    uint64_t x = *((uint64_t*)addr);

    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // second flush
    flush_cache_line(addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    fftimings[1] = count;
    // printf("cached: Used %lld instructions\n", count);
}

uint64_t flush_flush_threshold(void* addr, struct perf_event_attr pe, uint64_t count, int fd)
{   
    uint64_t fftimings [2];

    fftimings [0] = 0; // uncached flush
    fftimings [1] = 0; // cached flush 

    uint64_t uncached_flush_avg = 0;
    uint64_t cached_flush_avg = 0;

    FILE* fp;
    fp = fopen("fftest.txt", "w" );

    uint64_t ffuncached [1000];
    uint64_t ffcached [1000];
    // measure 1000 hits and misses and write to txt
    for(int i = 0; i < 1000; i++)
    {
        // get 1 hit and miss
        flush_flush_timing(addr, pe, count, fd, fftimings);
        // store in array to measure threshold
        ffuncached [i] = fftimings[0];
        ffcached [i] = fftimings[1];
        // write to txt for plotting
        fprintf(fp,"%lli\n", fftimings[0]);
        fprintf(fp,"%lli\n", fftimings[1]);
        uncached_flush_avg += fftimings[0];
        cached_flush_avg += fftimings[1];
    }

    fclose(fp);

    // following is unused but can be printed to console
    
    // calc avg
    uncached_flush_avg = uncached_flush_avg/1000;
    cached_flush_avg = cached_flush_avg/1000;
    

    // calc cached median
    qsort(ffcached, sizeof(ffcached)/sizeof(*ffcached), sizeof(*ffcached), comp);
    uint64_t median_c = (ffcached[1000/2]+ffcached[1000/2-1])/2;

    // calc uncached median
    qsort(ffuncached, sizeof(ffuncached)/sizeof(*ffuncached), sizeof(*ffuncached), comp);
    uint64_t median_uc = (ffuncached[1000/2]+ffuncached[1000/2-1])/2;
    
    return 0;
}

void flush_reload_timing(void* addr, struct perf_event_attr pe, uint64_t count, int fd, uint64_t* frtimings)
{
    // load data
    uint64_t x = *((uint64_t*)addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    // cached reload
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    x = *((uint64_t*)addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    frtimings[0] = count;

    // flush data

    flush_cache_line(addr);

    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    // uncached reload

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    x = *((uint64_t*)addr);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    frtimings[1] = count;
}

uint64_t flush_reload_threshold(void* addr, struct perf_event_attr pe, uint64_t count, int fd)
{
    uint64_t frtimings [2];

    frtimings [0] = 0; // cached reload
    frtimings [1] = 0; // uncached reload

    uint64_t cached_reload_avg = 0;
    uint64_t uncached_reload_avg = 0;

    FILE* fp2;
    fp2 = fopen("frtest.txt", "w" );

    uint64_t frcached [1000];
    uint64_t fruncached [1000];
    // measure 1000 hits and misses and write times to txt
    for(int i = 0; i < 1000; i++)
    {
        // get 1 hit and miss
        flush_reload_timing(addr, pe, count, fd, frtimings);
        // store in array to measure threshold
        frcached [i] = frtimings[0];
        fruncached [i] = frtimings[1];
        // write to txt for plotting
        fprintf(fp2,"%lli\n", frtimings[0]);
        fprintf(fp2,"%lli\n", frtimings[1]);
        cached_reload_avg += frtimings[0];
        uncached_reload_avg += frtimings[1];
    }

    fclose(fp2);
    // following is unused but can be printed to console

    // calc average
    cached_reload_avg = cached_reload_avg/1000;
    uncached_reload_avg = uncached_reload_avg/1000;

    // calc cached median
    qsort(frcached, sizeof(frcached)/sizeof(*frcached), sizeof(*frcached), comp);
    uint64_t median_c = (frcached[1000/2]+frcached[1000/2-1])/2;

    // calc uncached median
    qsort(fruncached, sizeof(fruncached)/sizeof(*fruncached), sizeof(*fruncached), comp);
    uint64_t median_uc = (fruncached[1000/2]+fruncached[1000/2-1])/2;
    
    return 0;
}