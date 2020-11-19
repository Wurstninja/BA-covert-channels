#include <stdio.h>
#include <stdint.h> // for uintN_t
#include "sharedmem.h"


void nop()
{
    asm volatile ("NOP");
}