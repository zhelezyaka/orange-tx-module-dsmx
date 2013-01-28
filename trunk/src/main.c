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
// generate DSMX channel list
void generateDSMXchannel_list(void){
unsigned long calc;
unsigned long channel;
unsigned char c = 0, k, part1 = 8, part2 = 7, part3 = 8, flag = 0;

    for(k = 0; k < 23; k++){
        channel_list[k] = 0xFF;//init channel list
    }

    //init randomize function
    calc = *(unsigned long *)&mnfctID[0];
	do{
		calc *= 0x0019660D;//randomize function
        calc += 0x3C6EF35F;//from wiki, liniux, gcc, etc

        channel = calc >> 8;
        channel = channel % 0x49;
//TX8_1_CPutString("raw=");TX8_1_PutSHexByte(channel + 3); TX8_1_PutChar(' ');
/*1.Если 4-й байт ManID - чётный ( "0" в младшем бите ), то и channel
должен быть таким же чётным ( или нечётным для нечётного 4-го байта ManID ).
Если он не такой - continue  ( генерируем в calc следующее число ).*/
//i need to compare last bit in manufacturerID and last bit in channel for both even/odd
        if( (channel & 1) == (mnfctID[3] & 1)){
/*2. Если channel совпадает с каким-либо из уже полученных ранее номеров
каналов в массиве, то continue */
			flag = 0;
            for(k = 0; k < 23; k++){
                if(channel_list[k] == (channel + 3)){
					flag++; k = 50;
				}
            }
			//TX8_1_CPutString("flag=");TX8_1_PutSHexByte(flag); TX8_1_PutChar(' ');
            if(flag == 0){
                flag = 1;
/*3. Проверяем, к какому поддиапазону channel относится ( 0..24, 25..48 или 49..72 ).
В начале работы - завести 3 счётчика ( для каждого поддиапазона ) и задать в них 8, 7 и 8
( 7 - в счётчике для поддиапазона 25..48 ).  В каком поддиапазоне channel - проверяем, не 0 ли
уже в соответствующем счётчике. Если 0 - continue*/
                if(channel < 25){
                    if(part1){
                        part1--;
                        flag = 0;
                    }
                }else{
                    if(channel < 49){
                        if(part2){
                            part2--;
                            flag = 0;
                        }
                    }else {
                        if(part3){
                            part3--;
                            flag = 0;
                        }
                    }
                }
                if(flag == 0){
/*Процедура вернёт, потом к этому ещё прибавится 4 ( в отличие от 2 для DSM2 ),
и с этим вызовет RadioSetFrequency, в которой отнимет ещё 1 и запишет в трансивер.*/
//i don`t use Cypress lib, write my self to CYRF. coz i need +3
                    channel_list[c] = channel + 3;//поэтому +3
//                    TX8_1_PutSHexByte(channel + 3); TX8_1_PutChar(' ');
                    c++;
                }
            }
        }

    }while(c < 23);
//	TX8_1_PutChar('/');TX8_1_PutCRLF();
}
//=====================================================================================================================
int main(void){


	init();

	CYRF_init(0);// normal mode
	CYRF_read_mnfctID();//после нее нужно добавить номер модели в mnfctID[3]

	generateDSMXchannel_list();


while(1){
}

return 0;
}