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
unsigned char work_mode = 0, max_channel_num = 8, power = 7;
unsigned int CRC_SEED;
unsigned char tflag = 0, tcount = 0;
unsigned char channelA = 0, channelB = 0;
unsigned char ortxTxBuffer[16+2], ortxRxBuffer[16+2];
unsigned char ortxTxISRIndex = 0, ortxRxISRIndex = 0, ortxTxBufferCount = 0;
unsigned char ortxTxStateMachine = 0;
unsigned int channelsData[14];

//=====================================================================================================================
void init(void){
	cli();

	//power On to all periphery
	PR.PRGEN = 0;
	PR.PRPC = 0;
	PR.PRPD = 0;

	//enable interrupt
	PMIC.CTRL = PMIC_HILVLEX_bm | PMIC_MEDLVLEX_bm | PMIC_LOLVLEX_bm;

	//setup PLL, Xtall 16 MHz	
	OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_256CLK_gc; // configure the XTAL input
	OSC.CTRL |= OSC_XOSCEN_bm; // start XTAL
	while (!(OSC.STATUS & OSC_XOSCRDY_bm)); // wait until ready
	OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | 0x2; // XTAL->PLL, 2x multiplier
	OSC.CTRL |= OSC_PLLEN_bm; // start PLL
	while (!(OSC.STATUS & OSC_PLLRDY_bm)); // wait until ready
	CCP = CCP_IOREG_gc; // allow changing CLK.CTRL
	CLK.PSCTRL = 0;//divA 1, divB 1, divC 1
	CCP = CCP_IOREG_gc; // allow changing CLK.CTRL
	CLK.CTRL = CLK_SCLKSEL_PLL_gc; // use PLL output as system clock

	//setup GPIO mode	
	PORTD.DIRSET = LED | CYRF_SCK | CYRF_MOSI | CYRF_SS;
	PORTD.DIRCLR = CYRF_MISO;
	PORTD.OUT |= LED | CYRF_SS;		//LED off	
	PORTE.DIRSET = CYRF_RESET;
	PORTE.DIRCLR = CYRF_IRQ;
	PORTE.OUT |= CYRF_RESET;
	
	//init SPI
	SPID.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_0_gc | SPI_PRESCALER_DIV64_gc;//msb firts

	//init USART0, portC
	PORTC.OUT |= 0x08;
	PORTC.DIRSET = 0x08;
	PORTC.DIRCLR = 0x04;
	USARTC0.BAUDCTRLA = 131;
	USARTC0.BAUDCTRLB = 0xD0;
//	USARTC0_CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_MED_gc | USART_DREINTLVL_OFF_gc;
//	USARTC0_CTRLB = USART_RXEN_bm | USART_TXEN_bm;
	USARTC0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
	USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	USARTC0.CTRLB = USART_RXEN_bm;
	
#ifdef DEBUG
	//init USART1, portC = PC6 - RXD1, PC7 - TXD1
	PORTC.OUT |= 0x80;	
	PORTC.DIRSET = 0x80;
	PORTC.DIRCLR = 0x40;	
	USARTC1.BAUDCTRLA = 131;//BSEL = 131 for 115200
	USARTC1.BAUDCTRLB = 0xD0;//BSCALE = -3
	USARTC1.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
	USARTC1.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	USARTC1.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
#endif	
	//init TIMER0
	TCC0.CTRLB = 0;
	TCC0.CTRLC = 0;
	TCC0.CTRLD = 0;
	TCC0.CTRLE = 0;
	TCC0.INTCTRLA = 3;
	TCC0.INTCTRLB = 0;
	TCC0.PER = 1000;
	TCC0.CTRLA = 1; //div 1

	sei();
}
//=====================================================================================================================
ISR(TCC0_OVF_vect){/* Overflow Interrupt */
	PORTD.OUT &= ~LED;		//LED on
	TCC0.PER = 1000;
	PORTD.OUT |= LED;		//LED off
}
//================================================================================================================================================
ISR(USARTC0_RXC_vect){/* Reception Complete Interrupt */
	if(ortxRxISRIndex < 18){
		ortxRxBuffer[ortxRxISRIndex++] = USARTC0.DATA;
	}
}
//================================================================================================================================================
ISR(USARTC0_TXC_vect){/* Transmission Complete Interrupt */
	if(ortxTxBufferCount){
		ortxTxBufferCount--;
		USARTC0.DATA = ortxTxBuffer[ortxTxISRIndex++];
	}
}
//================================================================================================================================================
void ortxTXTransmitBuffer(unsigned char i){
  while(ortxTxBufferCount);
  ortxTxISRIndex = 0;
  ortxTxBufferCount = i - 1;
  USARTC0.DATA = ortxTxBuffer[ortxTxISRIndex++];
}
//=====================================================================================================================
unsigned char spi(unsigned char data){
	SPID.DATA = data;
	while( ! (SPID.STATUS & SPI_IF_bm ) );	// Ensure the transmit buffer is free
	return SPID.DATA;
}
//=====================================================================================================================
int main(void){
unsigned char i;
unsigned char dsmX_channel_index, CRC_SEED_index = 0;

	init();
	
	//put_char(0xa5);
	put_string(" CYRF\r\n");	
//for(dsmX_channel_index=0;dsmX_channel_index<100;dsmX_channel_index++){for(i=0;i<255;i++)asm("nop");}
	print_hex8(0x8A);
//for(dsmX_channel_index=0;dsmX_channel_index<200;dsmX_channel_index++){for(i=0;i<255;i++)asm("nop");}
	print_hex8(0x73);

	CYRF_init(0);// normal mode
	

while(1){

	//analize incoming from UART
	if(ortxRxISRIndex == 18){
		if(ortxRxBuffer[0] == 0xAA){
			switch(ortxRxBuffer[1]){
			case 0x00:														//init
				work_mode = ortxRxBuffer[2];
				power = ortxRxBuffer[3];
				max_channel_num = ortxRxBuffer[4];
				CYRF_read_mnfctID();
				mnfctID[3] += ortxRxBuffer[5];//model number for model match feature
				
				if(work_mode & ORTX_BIND_FLAG){
					BIND_procedure();
					work_mode &= ~ORTX_BIND_FLAG;//clear BIND flag
				}
				
				CYRF_init(0);// normal mode
				
				if(work_mode & ORTX_USE_DSMX){		//dsmX mode
					generateDSMXchannel_list();
				}else{
					generateDSM2channel();
				}
				sop_col = (~mnfctID[0] + ~mnfctID[1] + ~mnfctID[2] + 2) & 7;
				data_col = 7 - sop_col;
				CRC_SEED = (mnfctID[0] << 8) + mnfctID[1]; 
			break;
			case 0x01:												//first 7 channel data
				for(i = 0; i < 7; i++){
					channelsData[i] = (ortxRxBuffer[i * 2 + 2] <<8) | ortxRxBuffer[i * 2 + 3];
				}
			break;
			case 0x02:												//second 7 channel data
				for(i = 0; i < 7; i++){
					channelsData[i + 7] = (ortxRxBuffer[i * 2 + 2] <<8) | ortxRxBuffer[i * 2 + 3];
				}
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
		}else{
			buildTransmitBuffer(0);
			transmit_receive(channelA, 0);
			TXbuffer[2] |= 0x80;
			transmit_receive(channelB, 1);		
		}
	}

}

return 0;
}