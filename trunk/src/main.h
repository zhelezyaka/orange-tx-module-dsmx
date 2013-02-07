#include <avr/io.h>
#include <avr/interrupt.h>

#define DEBUG 1

#include "cyrf.h"
#include "dsm.h"
#include "debug.h"

#define LED 0x02
#define ID_button 0x01
#define BIND_button 0x04

extern unsigned char mnfctID[6], sop_col, data_col;
extern unsigned char RXbuffer[0x10], TXbuffer[0x10];
extern unsigned char channel_list[23];
extern unsigned char work_mode, max_channel_num;
extern unsigned int CRC_SEED;
extern unsigned char tflag;
extern unsigned int tcount;
extern unsigned char channelA, channelB;
extern unsigned int channelsData[14];

unsigned char spi(unsigned char data);
void ortxTXTransmitBuffer(unsigned char i);