#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CRC16 0x8005

void map_ethernet_frame(uint8_t* ,uint16_t, char*);
void map_alternatingbits(uint8_t*);
uint16_t gen_crc16(const uint8_t*, uint16_t);
void set_sfd(uint8_t*);


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

    // payload has to be at least 46 characters and no more than 1500
    uint16_t extendedlength = payload_length;
    if(payload_length<46)
    {
        extendedlength = 46;
    }
    else if(payload_length>1501)
    {
        extendedlength = 1500;
        payload_length = 1500;
    }
    // fill buffer with mac header + payload characters (used later for crc)
    uint8_t* buffer = malloc(14+sizeof(uint8_t)*extendedlength);
    // fill with zeros
    for(int i = 0; i < (14 + extendedlength); i++)
    {
        buffer[i] = 0;
    }

    // map mac header
    // dst/src adress are filled with 0 bytes
    // -> fill first 12 bytes of buffer with zerotbytes
    for(int i = 0; i < 12; i++)
    {
        buffer[i] = 0;
    }
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

    // write payload length to buffer
    buffer[14] = 0;
    buffer[13] = 0;
    for(int i = 0; i < 8; i++)
    {
        buffer[14] += ethernet_frame[64+111-i]<<i;
        buffer[13] += ethernet_frame[64+111-i-8]<<i;

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

    // write payload to buffer
    for(int i = 0; i < payload_length; i++)
    {
        buffer[14+i] = payload [i];
    }

    // map checksum 

    uint16_t crc_16 = gen_crc16(buffer,14+extendedlength);
    for(int i = 0; i < 16; i++)
    {
        ethernet_frame[64 + 111 + extendedlength*8 + 32 - i] = 1&crc_16>>i;
    }
    
}

// for debugging
void map_alternatingbits(uint8_t* ethernet_frame)
{
    for(int i = 0; i < 10000000; i++)
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

/*  following code is copied from: 
    https://stackoverflow.com/questions/10564491/function-to-calculate-a-crc16-checksum
    */

uint16_t gen_crc16(const uint8_t *data, uint16_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;

    /* Sanity check: */
    if(data == NULL)
        return 0;

    while(size > 0)
    {
        bit_flag = out >> 15;

        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits

        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }

        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;

    }

    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }

    // item c) reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }

    return crc;
}