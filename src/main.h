#include <avr/io.h>
#include <avr/interrupt.h>

#define DEBUG 1

#include "cyrf.h"
#include "dsm.h"
#include "debug.h"

unsigned char mnfctID[6], sop_col, data_col;
unsigned char RXbuffer[0x10], TXbuffer[0x10];
unsigned char channel_list[23];
unsigned char work_mode, max_channel_num;
unsigned int CRC_SEED;
unsigned char tflag, tcount;
unsigned char channelA, channelB;
unsigned int channelsData[14];

unsigned char spi(unsigned char data);
void ortxTXTransmitBuffer(unsigned char i);