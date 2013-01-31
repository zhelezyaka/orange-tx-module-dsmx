#include "main.h"

#ifdef DEBUG

unsigned char debugTxBuffer[200], debugRxBuffer[30];
unsigned char debugTxISRIndex = 0, debugRxISRIndex = 0, debugTxBufferCount = 0;
const unsigned char hex[] = "0123456789ABCDEF";


ISR(USARTC1_RXC_vect){/* Reception Complete Interrupt */
	if(debugRxISRIndex < 30){
		debugRxBuffer[debugRxISRIndex++] = USARTC1_DATA;
	}
}
//================================================================================================================================================
ISR(USARTC1_TXC_vect){/* Transmission Complete Interrupt */
	if(debugTxBufferCount){
		debugTxBufferCount--;
		USARTC1_DATA = debugTxBuffer[debugTxISRIndex++];
	}
}
//================================================================================================================================================
void debugTXTransmitBuffer(unsigned char i){
  while(debugTxBufferCount);
  debugTxISRIndex = 0;
  debugTxBufferCount = i - 1;
  USARTC1_DATA = debugTxBuffer[debugTxISRIndex++];
}
//================================================================================================================================================
void put_char(unsigned char i){
  while(debugTxBufferCount);
  debugTxISRIndex = 0;
  debugTxBufferCount = 0;
  USARTC1_DATA = i;
}
//================================================================================================================================================
void print_hex8(unsigned char x){
  	debugTxBuffer[0] = '0';
	debugTxBuffer[1] = 'x';
	debugTxBuffer[2] = hex[(x>>4) & 15];
	debugTxBuffer[3] = hex[x & 15] ;	
	debugTXTransmitBuffer(4);
}
//================================================================================================================================================
void put_string(char *str){
unsigned char i=0;
  while (*str) {
    debugTxBuffer[i++] = *str++;
  }
  debugTXTransmitBuffer(i);
}
//================================================================================================================================================
#else
void print_hex8(unsigned char x){};
void put_string(char *str){};
void put_char(unsigned char i){};
#endif