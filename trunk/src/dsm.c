#include "main.h"

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
//=================================================================================================
void generateDSM2channel(void){
    channelA = (mnfctID[0] + mnfctID[2] + mnfctID[4]
                   + ((~mnfctID[0]) & 0xff) + ((~mnfctID[2]) & 0xff)) % 39 + 1;
    channelB = (mnfctID[1] + mnfctID[3] + mnfctID[5]
                   + ((~mnfctID[1]) & 0xff) + ((~mnfctID[3]) & 0xff)) % 40 + 40;
}
//=================================================================================================
unsigned char generateBINDchannel(void){
return 9;//true random number
}
//=================================================================================================
unsigned char BIND_procedure(void){
unsigned int temp_int;
unsigned char i, loop, error_code = 0;

	TXbuffer[0] = ~mnfctID[0]; TXbuffer[1] = ~mnfctID[1];
	TXbuffer[2] = ~mnfctID[2]; TXbuffer[3] = ~mnfctID[3];
	TXbuffer[4] = TXbuffer[0]; TXbuffer[5] = TXbuffer[1];
	TXbuffer[6] = TXbuffer[2]; TXbuffer[7] = TXbuffer[3];
	temp_int = 0x170;//CRC constant
	for(i = 0; i < 8; i++){
   		temp_int += TXbuffer[i];
	}
	TXbuffer[8] = temp_int >> 8; TXbuffer[9] = temp_int & 0xFF;
	TXbuffer[10] = 0x01;//Unknown (value = 0x01)
	TXbuffer[11] = max_channel_num;//# of channels being transmitted			
	if(max_channel_num < 8){
		if (work_mode & ORTX_USE_DSMX){
			TXbuffer[12] = 0xA2;
		}else{
			TXbuffer[12] = 0x01;
		}
	}else{
		if (work_mode & ORTX_USE_DSMX){
			TXbuffer[12] = 0xB2;
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
	loop = 100;
	do{
		loop--;
		CYRF_init(ORTX_BIND_FLAG);// bind mode
		if( transmit_receive(i , ORTX_BIND_FLAG) == ORTX_BIND_FLAG){
			loop = 0;
			error_code = 1;
		}
	}while(loop);
return error_code;
}
//=================================================================================================
unsigned char transmit_receive(unsigned char channel, unsigned char control){
unsigned char row, rssi, rx_irq_status, rx_count, error_code = 0;

	if(control == 0 || control == 1){				//if no BIND mode
		if(work_mode & ORTX_USE_DSMX){
			row = (channel - 2) % 5;//ok!
		}else{
			row = channel % 5;
		}

		if(control){
			CYRF_write(0x15, ~(CRC_SEED & 0xFF));
			CYRF_write(0x16, ~(CRC_SEED >> 8));
		}else{
			CYRF_write(0x15, CRC_SEED & 0xFF);
			CYRF_write(0x16, CRC_SEED >> 8);
		}
		CYRF_write_block_const(0xA2, &pncodes[row][sop_col][0], 8);		// load SOP
		CYRF_write_block_const(0xA3, &pncodes[row][data_col][0], 8);	//load DATA
		CYRF_write_block_const(0xA3, &pncodes[row][data_col+1][0], 8);
	}
	
	CYRF_write(0x80, channel);//  - wr CHANNEL_ADR	

	//transmit packet
	//CYRF_write(0x0E, 0x80 | 0x20);//XOUT and PACTL - high
	CYRF_write(0x02, 0x40); //TX_CTRL_ADR = TX CLR				
	CYRF_write_block(0x20, TXbuffer, 0x10);
	CYRF_write(0x01, 0x10); //TX_LENGTH_ADR
	CYRF_write(0x02, 0x82); //TX_CTRL_ADR
	//подождем пока все уйдет
//		tcount = 20; // 0,5 mSec
//		tflag = 1;// устанавливаем флаг
//		do{
//			if(CYRF_read(0x04) & 0x02){ tflag = 0; tcount = 0;}
//		}while( tflag );
	while( ! (CYRF_read(0x04) & 0x02) );
	//CYRF_write(0x0E,0x00);//all GPIO - low
	
	//todo: check for error transmition

	if(control == ORTX_BIND_FLAG){
		CYRF_write_block_const(0xA3, &pn_bind[0], 8);
	}
	//receive packet
	CYRF_write(0x85, 0x83);//  - wr RX_CTRL_ADR = RX GO | RXC IRQEN | RXE IRQEN 
	rssi = CYRF_read(0x13);//  - rd RSSI_ADR 
	
	//CYRF_write(0x0E, 0x80);//XOUT = high	
	//ждем пока не произойдет прерывание
	//if(control & ORTX_BIND_FLAG){
	//tcount = 25; // 12 mSec in BIND mode
	//}else{
	//tcount = 10; // 1 mSec in normal mode telemetry answer
	//}
	//tflag = 1;// устанавливаем флаг	
	do{
		if(PORTE_IN & CYRF_IRQ){ tflag = 0; tcount = 0;}
	}while( tflag );

	//CYRF_write(0x0E, 0x00);//XOUT = low
	
	if(PORTE_IN & CYRF_IRQ){		//прерывание было
		rssi = CYRF_read(0x13);//  - rd RSSI_ADR 
		rx_irq_status = CYRF_read(0x07);
		//был принят пакет
		if(rx_irq_status & RXC){
//				LED_Data_ADDR &= ~LED_MASK;	//ON
			rx_count = CYRF_read(0x09);// - rd RX_COUNT_ADR
			CYRF_read_block(0x21, RXbuffer, rx_count);//получим принятый пакет в буфер
			if(rx_count == 0x10){
				error_code = 1;
			}
			if(control == ORTX_BIND_FLAG && rx_count == 10){
				error_code = ORTX_BIND_FLAG;
			}
		}else{//была ошибка приема	
			CYRF_write(0xA9, 0x20);// - wr RX_ABORT_ADR = ABORT EN
			//пауза нужна небольшая
			//LED_Data_ADDR |= LED_MASK;//OFF
			CYRF_write(0xA9, 0x00);// - wr RX_ABORT_ADR
		}
	}else{//не было прерывания
		CYRF_write(0xA9, 0x20);// - wr RX_ABORT_ADR = ABORT EN
		CYRF_write(0x8F, 0x2C);// - wr XACT_CFG_ADR = FRC END | Synth Mode (RX) 
		while(CYRF_read(0x0F) & 0x20 );
		CYRF_write(0xA9, 0x00);// - wr RX_ABORT_ADR*/
	}	

	return error_code;
}
//=================================================================================================
void buildTransmitBuffer(unsigned char top){
unsigned char i;
	if(work_mode & ORTX_USE_DSMX){
		TXbuffer[0] = mnfctID[2]; TXbuffer[1] = mnfctID[3];
	}else{
		TXbuffer[0] = ~mnfctID[2]; TXbuffer[1] = ~mnfctID[3];
	}

	if(top == 0){
		for(i = 0; i < max_channel_num; i++){
		}
		for(i = max_channel_num; i < 7;i++){
			TXbuffer[i * 2 + 2] = 0xFF;//not used channels
			TXbuffer[i * 2 + 3] = 0xFF;
		}
	}else{
			//build transmit data for second seven channels
			for(i = 7; i < max_channel_num; i++){
			}
			for(i = max_channel_num; i < 14; i++){
				TXbuffer[(i-7) * 2 + 2] = 0xFF;//not used channels
				TXbuffer[(i-7) * 2 + 3] = 0xFF;
			}
	}
}