#include <avr/io.h>
#include <avr/interrupt.h>

#include "cyrf.h"

unsigned char mnfctID[6], sop_col, data_col;
unsigned char RXbuffer[0x10], TXbuffer[0x10];
unsigned char channel_list[23];

unsigned char spi(unsigned char data);