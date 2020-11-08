void flush_flush_timing(void* addr, struct perf_event_attr pe, uint64_t count, int fd, uint64_t* fftimings)
{
    // first flush
    flush_cache_line(addr);

    // not caching data

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // second flush
    flush_cache_line(addr);

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    fftimings[0] = count;

    // printf("not cached: Used %lld instructions\n", count);

    // first flush
    flush_cache_line(addr);

    // cache data

    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));

    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // second flush
    flush_cache_line(addr);

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

    printf("Flush+Flush test:\n");
    FILE* fp;
    fp = fopen("fftest.txt", "w" );

    uint64_t ffuncached [10000];
    uint64_t ffcached [10000];

    for(int i = 0; i < 10000; i++)
    {
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

    uncached_flush_avg = uncached_flush_avg/10000;
    cached_flush_avg = cached_flush_avg/10000;

    fclose(fp);

    printf("%lli, %lli\n", uncached_flush_avg, cached_flush_avg);

    // calculating threshold

    // find min cached flush
    uint64_t ffmin = 1000;
    for(int i = 0; i<10000; i++)
    {
        if(ffmin>ffcached[i])
        {
            ffmin = ffcached[i];
        }
    }

    return ffmin-1;
}

void flush_reload_timing(void* addr, struct perf_event_attr pe, uint64_t count, int fd)
{
    // load data
    
    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    // cached reload
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));

    frtimings[0] = count;

    // flush data

    flush_cache_line(addr);

    // uncached reload

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    // data, instruction barrier
    asm volatile ("DSB SY");
    asm volatile ("ISB");

    asm volatile ("MOV X0, %0;"
                    :: "r"  (buffer[10]));
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

    printf("Flush+Reload test:\n");

    FILE* fp2;
    fp2 = fopen("frtest.txt", "w" );

    uint64_t frcached [10000];
    uint64_t fruncached [10000];

    for(int i = 0; i < 10000; i++)
    {
        flush_reload_test(addr, pe, count, fd, frtimings);
        // store in array to measure threshold
        frcached [i] = frtimings[0];
        fruncached [i] = frtimings[1];
        // write to txt for plotting
        fprintf(fp2,"%lli\n", frtimings[0]);
        fprintf(fp2,"%lli\n", frtimings[1]);
        cached_reload_avg += frtimings[0];
        uncached_reload_avg += frtimings[1];
    }

    

    cached_reload_avg = cached_reload_avg/10000;
    uncached_reload_avg = uncached_reload_avg/10000;

    fclose(fp2);

    printf("%lli, %lli\n", cached_reload_avg, uncached_reload_avg);

    // find min uncached reload
    uint64_t frmin = 1000;
    for(int i = 0; i<10000; i++)
    {
        // only works if no uncached reload happens to be a cached reload, therefore check if below 300
        if(frmin>fruncached[i]&&fruncached[i]>300)
        {
            frmin = fruncached[i];
        }
    }
    
    return frmin-1;
}