#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void map_ethernet_frame(uint8_t* ,uint16_t, char*);
void set_sfd(uint8_t*);
void map_alternatingbits(uint8_t*);

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

void map_ethernet_frame(uint8_t* ethernet_frame ,uint16_t payload_length, char* payload)
{
    // map preamble
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

    // map sfd
    ethernet_frame [56] = 1;
    ethernet_frame [56+1] = 0;
    ethernet_frame [56+2] = 1;
    ethernet_frame [56+3] = 0;
    ethernet_frame [56+4] = 1;
    ethernet_frame [56+5] = 0;
    ethernet_frame [56+6] = 1;
    ethernet_frame [56+7] = 1;

    // map mac header
    // no need for dst/src adress
    // set ether type with length of payload
    uint16_t length = payload_length;
    for(int i = 0; i < 16; i++)
    {
        if(length%2) // if LSB is 1
        {
            ethernet_frame[64+111-i] = 1;
        }
        else
        {
            ethernet_frame[64+111-i] = 0;
        }
        length=length>>1;
    }

    // map payload
    int cur;
    length = payload_length;
    // for every symbol
    for(int i = 0; i < length; i++)
    {
        cur = (int)payload[i];
        // store the ascii bits in ethernet_frame
        for(int k = 0; k < 8; k++)
        {
            if(cur%2) // if LSB is odd
            {
                // write bits to corresponding bit position in ethernet_frame
                ethernet_frame[64 + 111 + i*8 + 8-k] = 1;
            }
            else
            {
                // write bits to corresponding bit position in ethernet_frame
                ethernet_frame[64 + 111 + i*8 + 8-k] = 0;
            }
            cur=cur>>1;
        }
    }

    // map checksum 
}

// for debugging
void map_alternatingbits(uint8_t* ethernet_frame)
{
    for(int i = 0; i < 1000; i++)
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