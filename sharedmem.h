#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h> // for shm
#include <sys/stat.h> // for modes shm
#include <fcntl.h> // for O_ flags shm
#include <stdint.h>
#include <unistd.h>

#define SIZE 192

extern uint64_t* sharedmem; 

void init_sharedmem();
void open_sharedmem();