#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void set_preamble(uint8_t*);
void set_sfd(uint8_t*);
void set_macheader(uint8_t*, uint16_t);

void set_preamble(uint8_t* preamble)
{
    for(int i = 0; i < 56; i++)
    {
        if(i%2) // if bit is odd
        {
            preamble [i] = 0;
        }
        else
        {
            preamble [i] = 1;
        }
        
    }
}

void set_sfd(uint8_t* sfd)
{
    sfd [0] = 1;
    sfd [1] = 0;
    sfd [2] = 1;
    sfd [3] = 0;
    sfd [4] = 1;
    sfd [5] = 0;
    sfd [6] = 1;
    sfd [7] = 1;
}

void set_macheader(uint8_t* macheader, uint16_t length)
{
    // no need for dst/src adress
    //set ether type with length of payload
    for(int i = 0; i < 16; i++)
    {
        if(length%2) // if LSB is 1
        {
            macheader[111-i] = 1;
        }
        else
        {
            macheader[111-i] = 0;
        }
        length=length>>1;
    }
    
}
// TODO
void map_ethernet_frame(uint8_t* ethernet_frame)
{
    for(int i = 0; i < 56; i++)
    {
        if(i%2) // if bit is odd
        {
            ethernet_frame [i] = 0;
        }
        else
        {
            ethernet_frame [i] = 1;
        }
        
    }
    
}