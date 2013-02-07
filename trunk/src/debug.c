#include "main.h"

#ifdef DEBUG

unsigned char /*debugTxBuffer[200],*/ debugRxBuffer[30];
unsigned char /*debugTxISRIndex = 0,*/ debugRxISRIndex = 0, debugTxBufferCount = 0;
const unsigned char hex[] = "0123456789ABCDEF";

ISR(USARTC1_RXC_vect){// Reception Complete Interrupt
	if(debugRxISRIndex < 30){
		debugRxBuffer[debugRxISRIndex++] = USARTC1.DATA;
	}
}
//================================================================================================================================================
/*ISR(USARTC1_DRE_vect){// Data Register Empty Interrupt
uint8_t tempCTRLA;
	if(debugTxBufferCount ){
		USARTC1.DATA = debugTxBuffer[debugTxISRIndex++];
		debugTxBufferCount--;
	}else{
		//tempCTRLA = USARTC1.CTRLA;
		//tempCTRLA = (tempCTRLA & ~USART_DREINTLVL_gm) | USART_DREINTLVL_OFF_gc;
		//USARTC1.CTRLA = tempCTRLA;
		USARTC1.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
		debugTxBufferCount = 0;
	}
}
//================================================================================================================================================
void debugTXTransmitBuffer(unsigned char i){
uint8_t tempCTRLA;
  while(debugTxBufferCount);
  debugTxISRIndex = 0;
  debugTxBufferCount = i;
// 	USARTC1.DATA = debugTxBuffer[debugTxISRIndex++]; 
	tempCTRLA = USARTC1.CTRLA;
	tempCTRLA = (tempCTRLA & ~USART_DREINTLVL_gm) | USART_DREINTLVL_MED_gc;
	USARTC1.CTRLA = tempCTRLA;
}*/
//================================================================================================================================================
void put_char(unsigned char i){
while(!(USARTC1.STATUS & USART_DREIF_bm));
USARTC1.DATA = i;
/*uint8_t tempCTRLA;
   while(debugTxBufferCount);
  debugTxISRIndex = 0;
  debugTxBufferCount = 1;
  debugTxBuffer[0] = i;
	tempCTRLA = USARTC1.CTRLA;
	tempCTRLA = (tempCTRLA & ~USART_DREINTLVL_gm) | USART_DREINTLVL_MED_gc;
	USARTC1.CTRLA = tempCTRLA;*/
}
//================================================================================================================================================
void print_hex8(unsigned char x){
put_char('0');
put_char('x');
put_char(hex[ x>>4 ]);
put_char(hex[ x & 0x0F ] );
put_char(' ');
/*	while(debugTxBufferCount); 
 	debugTxBuffer[0] = '0';
	debugTxBuffer[1] = 'x';
	debugTxBuffer[2] = hex[ x>>4 ];
	debugTxBuffer[3] = hex[ x & 0x0F ] ;	
	debugTXTransmitBuffer(4);*/
}
//================================================================================================================================================
void put_string(char *str){
//unsigned char i=0;
//	while(debugTxBufferCount); 
  while (*str) {
    //debugTxBuffer[i++] = *str++;
	put_char(*str++);
  }
  //debugTXTransmitBuffer(i);
}
//================================================================================================================================================
#else
void print_hex8(unsigned char x){};
void put_string(char *str){};
void put_char(unsigned char i){};
#endif