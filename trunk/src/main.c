/* default pinouts of module

PC2 - RXD0 \
PC3 - TXD0 / to PPM input
PC5 - buzzer
PD0 - button
PD1 - LED to VCC
PD2 - button

PE1 - CYRF IRQ
PE0 - CYRF reset
PD7 - CYRF SCK
PD6 - CYRF MISO
PD5 - CYRF MOSI
PD4 - CYRF SS
PD3 - inverted PPM input

*/

#include "main.h"

unsigned char mnfctID[6], sop_col, data_col;
unsigned char RXbuffer[0x10], TXbuffer[0x10];
unsigned char channel_list[23];
unsigned char work_mode = 0, max_channel_num = 8;
unsigned int CRC_SEED;
unsigned char tflag = 0, tcount = 0;
unsigned char channelA = 0, channelB = 0;
unsigned char ortxTxBuffer[16+2], ortxRxBuffer[16+2];
unsigned char ortxTxISRIndex = 0, ortxRxISRIndex = 0, ortxTxBufferCount = 0, ortxRxBufferCount = 0;
unsigned char ortxTxStateMachine = 0;

//=====================================================================================================================
void init(void){
	cli();
	//setup GPIO mode
	PORTE_DIRSET |= CYRF_RESET;
	PORTE_DIRCLR &= ~CYRF_IRQ;
	PORTD_DIRSET |= CYRF_SCK;
	PORTD_DIRSET |= CYRF_MOSI;
	PORTD_DIRSET |= CYRF_SS;
	PORTD_DIRCLR &= ~CYRF_MISO;
	PORTD_OUT |= CYRF_SS;
	PORTE_OUT |= CYRF_RESET;

	//init SPI
	SPID_CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_0_gc | SPI_PRESCALER_DIV64_gc;//msb firts

	//init USART0, portC
	USARTC0_CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_MED_gc | USART_DREINTLVL_OFF_gc;
	USARTC0_CTRLB = USART_RXEN_bm | USART_TXEN_bm;
	USARTC0_CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	USARTC0_BAUDCTRLA = 0;
	USARTC0_BAUDCTRLB = 0;
	
	sei();
}
//=====================================================================================================================
unsigned char spi(unsigned char data){
	SPID_DATA = data;
	while( ! (SPID_STATUS & SPI_IF_bm ) );	// Ensure the transmit buffer is free
	return SPID_DATA;
}
//=====================================================================================================================
int main(void){
//unsigned char i;
unsigned char dsmX_channel_index, CRC_SEED_index = 0;

	init();

	CYRF_init(0);// normal mode
	

while(1){

	//analize incoming from UART
	if(ortxRxISRIndex == 18){
		if(ortxRxBuffer[0] == 0xAA){
			switch(ortxRxBuffer[1]){
			case 0x00:														//init
				CYRF_read_mnfctID();
				mnfctID[3] += ortxRxBuffer[5];//model number for model match feature
				if(ortxRxBuffer[2] & ORTX_USE_DSMX){		//dsmX mode
					generateDSMXchannel_list();
				}else{
					generateDSM2channel();
				}
				sop_col = (~mnfctID[0] + ~mnfctID[1] + ~mnfctID[2] + 2) & 7;
				data_col = 7 - sop_col;
				CRC_SEED = (mnfctID[0] << 8) + mnfctID[1]; 
			break;
			}
		}
		ortxRxISRIndex = 0;
	}
	
	//transmit data packet and receive telemetry answer
	if(work_mode & ORTX_USE_DSMX){
//DSMX mode
		for(dsmX_channel_index = 0; dsmX_channel_index < 23; dsmX_channel_index++){
			transmit_receive(channel_list[dsmX_channel_index], CRC_SEED_index);
		}
		if(CRC_SEED_index)CRC_SEED_index = 0;
		else CRC_SEED_index = 1;
		//промежканальная пауза
	}else{
//DSM2 mode	
		//build transmit data for first seven channels
		buildTransmitBuffer(0);
		transmit_receive(channelA, 0);
		// - пауза между каналами 4 мсек
		TXbuffer[2] |= 0x80;
		transmit_receive(channelB, 1);
		//далее большая пауза
		if(max_channel_num > 7){
			buildTransmitBuffer(1);
			transmit_receive(channelA, 0);
			// - пауза между каналами 4 мсек
			transmit_receive(channelB, 1);
			//далее большая пауза		
		}
	}

}

return 0;
}