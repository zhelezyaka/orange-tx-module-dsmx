#include "main.h"
int vvval=0;
//=====================================================================================================================
// generate DSMX channel list
void generateDSMXchannel_list(void){
unsigned long calc;
unsigned long channel;
unsigned char c = 0, k, part1 = 8, part2 = 7, part3 = 8, flag = 0;

put_string("\r\ndsmX = ");

    for(k = 0; k < 23; k++){
        channel_list[k] = 0xFF;//init channel list
    }

    //init randomize function
    //calc = *(unsigned long *)&mnfctID[0];
calc = mnfctID[0];
calc = calc << 8;
calc |= mnfctID[1];
calc = calc << 8;
calc |= mnfctID[2];
calc = calc << 8;
calc |= mnfctID[3];

calc = 0xFFFFFFFF - calc;
print_hex8(calc>>24);print_hex8(calc>>16);print_hex8(calc>>8);print_hex8(calc);put_string(" = ");
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
        if( (channel & 1) != (mnfctID[3] & 1)){
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
                    print_hex8(channel + 3);
                    c++;
                }
            }
        }

    }while(c < 23);
	put_string("\r\n");
}
//=================================================================================================
void generateDSM2channel(void){/*
    channelA = (mnfctID[0] + mnfctID[2] + mnfctID[4]
                   + ((~mnfctID[0]) & 0xff) + ((~mnfctID[2]) & 0xff)) % 39 + 1;
    channelB = (mnfctID[1] + mnfctID[3] + mnfctID[5]
                   + ((~mnfctID[1]) & 0xff) + ((~mnfctID[3]) & 0xff)) % 40 + 40;*/
				   
	channelA = 8;
	channelB = 40;
}
//=================================================================================================
unsigned char generateBINDchannel(void){
return 9;//true random number
}
//=================================================================================================
unsigned char BIND_procedure(void){
unsigned int temp_int, loop;
unsigned char i, error_code = 0;

	TXbuffer[0] = (0xFF - mnfctID[0]); TXbuffer[1] = (0xFF - mnfctID[1]);
	TXbuffer[2] = (0xFF - mnfctID[2]); TXbuffer[3] = (0xFF - mnfctID[3]);
	TXbuffer[4] = TXbuffer[0]; TXbuffer[5] = TXbuffer[1];
	TXbuffer[6] = TXbuffer[2]; TXbuffer[7] = TXbuffer[3];
	temp_int = 0x170;//CRC constant
	for(i = 0; i < 8; i++){
   		temp_int += TXbuffer[i];
	}
	TXbuffer[8] = temp_int >> 8; TXbuffer[9] = temp_int & 0xFF;
	TXbuffer[10] = 0x01;//Unknown (value = 0x01)
	TXbuffer[11] = max_channel_num;//# of channels being transmitted			
//0xB2 - 11 msec
	if(max_channel_num < 8){
		if (work_mode & ORTX_USE_DSMX){
			TXbuffer[12] = 0xb2;
		}else{
			TXbuffer[12] = 0x01;
		}
	}else{
		if (work_mode & ORTX_USE_DSMX){
			TXbuffer[12] = 0xb2;
		}else{
			TXbuffer[12] = 0x02;
		}
	}	
	TXbuffer[13] = 0x00;//Unknown (value = 0x00)
	for(i = 8; i < 14; i++){
   		temp_int += TXbuffer[i];
	}
	TXbuffer[14] = temp_int >> 8; TXbuffer[15] = temp_int & 0xFF;
	
	i = generateBINDchannel();
	loop = 200;
	do{
		loop--;
		CYRF_init(ORTX_BIND_FLAG);// bind mode
		if( transmit_receive(i , ORTX_BIND_FLAG) == ORTX_BIND_FLAG){
//			loop = 0;
			error_code = 1;
		}
	}while(loop);
	
	max_channel_num = 6;
return error_code;
}
//=================================================================================================
unsigned char transmit_receive(unsigned char channel, unsigned char control){
unsigned char row, rssi, rx_irq_status, rx_count, error_code = 0, XACT_CFG_ADR;
#ifdef DEBUG
unsigned char i;
#endif

	if(control == 0 || control == 1){				//if no BIND mode
		if(work_mode & ORTX_USE_DSMX){
			row = (channel - 2) % 5;
		}else{
			row = channel % 5;
		}

		if(control){
			CYRF_write(0x15, mnfctID[1]);
			CYRF_write(0x16, mnfctID[0]);
		}else{
			CYRF_write(0x15, 0xff - mnfctID[1]);
			CYRF_write(0x16, 0xff - mnfctID[0]);
		}

		CYRF_write_block_const(0xA2, &pncodes[row][sop_col][0], 8);		// load SOP
		CYRF_write_block_const(0xA3, &pncodes[row][data_col][0], 8);	//load DATA
		CYRF_write_block_const(0xA3, &pncodes[row][data_col+1][0], 8);
	}
	
	CYRF_write(0x80, channel);//  - wr CHANNEL_ADR	
	
	PORTD.OUTSET = LED;		//LED off
	
	//transmit packet
	CYRF_write(0x0E,0x20);//PACTL
	
	CYRF_write(0x02, 0x40); //TX_CTRL_ADR = TX CLR				
	CYRF_write_block(0x20, TXbuffer, 0x10);
	CYRF_write(0x01, 0x10); //TX_LENGTH_ADR
	CYRF_write(0x02, 0x83); //TX_CTRL_ADR = TX GO | TXC | TXE
	//подождем пока все уйдет
	if(control == ORTX_BIND_FLAG){
		tcount = 120; // 12 mSec
	}else{
		tcount = 10; // 1 mSec
	}
	tflag = 1;// устанавливаем флаг
	do{
		if(CYRF_read(0x04) & 0x02){ tcount = 0; /*tflag = 0;*/ }
	}while( tflag);

	CYRF_write(0x0E, 0);//XOUT and PACTL - low

	PORTD.OUTCLR = LED;		//LED on
	//todo: check for error transmition

	if(control == ORTX_BIND_FLAG){
		CYRF_write_block_const(0xA3, &pn_bind[0], 8);
	}

	//receive packet
	CYRF_write(0x85, 0x83);//  - wr RX_CTRL_ADR = RX GO | RXC IRQEN | RXE IRQEN 
	rssi = CYRF_read(0x13);//  - rd RSSI_ADR 
	
	PORTD.OUTSET = LED;		//LED off

	//ждем пока не произойдет прерывание
	if(control & ORTX_BIND_FLAG){
		tcount = 120; // 12 mSec in BIND mode
	}else{
		tcount = 20; // 1 mSec in normal mode telemetry answer
	}
	tflag = 1;// устанавливаем флаг	
	do{
		if(PORTE.IN & CYRF_IRQ){ tcount = 0; tflag = 0;}
	}while( tflag );

	PORTD.OUTCLR = LED;		//LED on

	if(PORTE.IN & CYRF_IRQ){		//прерывание было
		rssi = CYRF_read(0x13);//  - rd RSSI_ADR 
		rx_irq_status = CYRF_read(0x07);
		//был принят пакет
		if(rx_irq_status & RXC){
			rx_count = CYRF_read(0x09);// - rd RX_COUNT_ADR
			
			CYRF_read_block(0x21, RXbuffer, rx_count);//получим принятый пакет в буфер
			
			#ifdef DEBUG
				if(control == ORTX_BIND_FLAG && rx_count > 0 && rx_irq_status){
					//print_hex8(channel); put_char(' ');
					print_hex8(rssi); put_char(' ');
					print_hex8(rx_irq_status); put_char(' ');
					print_hex8(CYRF_read(0x08)); put_char(' ');//RX_STATUS_ADR
					print_hex8(CYRF_read(0x04)); put_char(' ');//TX_IRQ_STATUS_ADR
					print_hex8(rx_count); put_char('[');
					for(i = 0; i < rx_count; i++){
						print_hex8(RXbuffer[i]);
						if( (i < rx_count-1) && (i%2) )put_char('-');
					}
					put_string("]\r\n");
				}
			#endif			

			if(rx_count == 0x10 /*&& (rx_irq_status & RXE) */){
				error_code = ORTX_USE_TM;	//telemetry answer
			}
			
			if(control == ORTX_BIND_FLAG && rx_count == 10//  && !(rx_irq_status & RXE)
				&& RXbuffer[0] == (0xFF - mnfctID[0]) && RXbuffer[1] == (0xFF - mnfctID[1]) ){
				put_string("bind ok\r\n");
				error_code = ORTX_BIND_FLAG;
			}
		}else{//была ошибка приема	
			//CYRF_write(0x29, 0x20);// - wr RX_ABORT_ADR = ABORT EN
			//пауза нужна небольшая
			CYRF_write(0x8F, 0x2C);// - wr XACT_CFG_ADR = FRC END | Synth Mode (RX) 
			while(CYRF_read(0x0F) & 0x20 );
			//CYRF_write(0x29, 0x00);// - wr RX_ABORT_ADR
		}
	}else{//не было прерывания
		//CYRF_write(0x29, 0x20);// - wr RX_ABORT_ADR = ABORT EN
		CYRF_write(0x0F, 0x2C);// - wr XACT_CFG_ADR = FRC END | Synth Mode (RX) 
		do{
			XACT_CFG_ADR = CYRF_read(0x0F);
		}while(XACT_CFG_ADR & 0x20 );
		//CYRF_write(0x29, 0x00);// - wr RX_ABORT_ADR
	}	

	return error_code;
}
//=================================================================================================
void buildTransmitBuffer(unsigned char top){
//unsigned char i;
	if(work_mode & ORTX_USE_DSMX){
		TXbuffer[0] = mnfctID[2]; TXbuffer[1] = mnfctID[3];
	}else{
		TXbuffer[0] = (0xFF - mnfctID[2]); TXbuffer[1] = (0xFF - mnfctID[3]);
	}
/*
	if(top == 0){
		for(i = 0; i < max_channel_num; i++){
			TXbuffer[i * 2 + 2] = channelsData[i] >>8;
			TXbuffer[i * 2 + 3] = channelsData[i] & 0xFF;
		}
		for(i = max_channel_num; i < 7;i++){
			TXbuffer[i * 2 + 2] = 0xFF;//not used channels
			TXbuffer[i * 2 + 3] = 0xFF;
		}
	}else{
			//build transmit data for second seven channels
		for(i = 7; i < max_channel_num; i++){
			TXbuffer[(i-7) * 2 + 2] = channelsData[i] >>8;
			TXbuffer[(i-7) * 2 + 3] = channelsData[i] & 0xFF;
		}
		for(i = max_channel_num; i < 14; i++){
			TXbuffer[(i-7) * 2 + 2] = 0xFF;//not used channels
			TXbuffer[(i-7) * 2 + 3] = 0xFF;
		}
	}
*/
	if(work_mode & ORTX_USE_11bit){
		if(top){
			TXbuffer[2]= (7<<3) | 0x84; TXbuffer[3]=0x00;
			TXbuffer[4]= (8<<3) | 0x04; TXbuffer[5]=0x00;
			TXbuffer[6]= (9<<3) | 0x04; TXbuffer[7]=0x00;
			TXbuffer[8]= (10<<3) | 0x04; TXbuffer[9]=0x00;
			TXbuffer[10]=(11<<3) | 0x04; TXbuffer[11]=0x00;
			TXbuffer[12]=(12<<3) | 0x04; TXbuffer[13]=0x00;
			TXbuffer[14]=(13<<3) | 0x04; TXbuffer[15]=0x00;
		}else{
			vvval++;
			if(vvval >= 2047)vvval = 0;
			TXbuffer[2]= (0<<3) | 0x04; TXbuffer[3]=0x00;
			TXbuffer[4]= (1<<3) | (vvval >>8); TXbuffer[5]=vvval;
			TXbuffer[6]= (2<<3) | 0x04; TXbuffer[7]=0x00;
			TXbuffer[8]= (3<<3) | 0x04; TXbuffer[9]=0x00;
			TXbuffer[10]= (4<<3) | 0x04; TXbuffer[11]=0x00;
			TXbuffer[12]= (5<<3) | 0x04; TXbuffer[13]=0x00;
			TXbuffer[14]= (6<<3) | 0x04; TXbuffer[15]=0xff;	
		}
	}else{
		if(top){
			TXbuffer[2]= (7<<2) | 0x02; TXbuffer[3]=0x00;
			TXbuffer[4]= (8<<2) | 0x02; TXbuffer[5]=0x00;
			TXbuffer[6]= (9<<2) | 0x02; TXbuffer[7]=0x00;
			TXbuffer[8]= (10<<2) | 0x02; TXbuffer[9]=0x00;
			TXbuffer[10]= (11<<2) | 0x02; TXbuffer[11]=0x00;
			TXbuffer[12]= (12<<2) | 0x02; TXbuffer[13]=0x00;
			TXbuffer[14]= (13<<2) | 0x02; TXbuffer[15]=0x00;
		}else{
			TXbuffer[2]= (0<<2) | 0x02; TXbuffer[3]=0x00;
			TXbuffer[4]= (1<<2) | 0x02; TXbuffer[5]=0x00;
			TXbuffer[6]= (2<<2) | 0x02; TXbuffer[7]=0x00;
			TXbuffer[8]= (3<<2) | 0x02; TXbuffer[9]=0x00;
			TXbuffer[10]= (4<<2) | 0x02; TXbuffer[11]=0x00;
			TXbuffer[12]= (5<<2) | 0x02; TXbuffer[13]=0x00;
			TXbuffer[14]= (6<<2) | 0x02; TXbuffer[15]=0x00;
		}
	}
}
/*
	mnfctID[0] = 0x6d; mnfctID[1] = 0x39; mnfctID[2] = 0xa7; mnfctID[3] = 0xF5; //my
ответ при BIND:
09 BC 5A 17 B8 0A[92C6-580A-0106-0100-0332]
chan 0x16 0x43
sop_col 07
CRC_SEED 92C6


*/