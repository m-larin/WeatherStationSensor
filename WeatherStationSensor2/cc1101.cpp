/*
 * CPPFile1.cpp
 *
 * Created: 11.12.2018 1:03:26
 *  Author: Mikhail
 */ 
#include <util/delay.h>
#include "spi.h"
#include "cc1101.h"

//Записать регистр
void cc1101_write_reg(unsigned char regAddress, unsigned char regValue){
	 spi_sendbyte(regAddress);
	 spi_sendbyte(regValue);
}

//Отправить команду
unsigned char cc1101_strobe(unsigned char strobe){
	return spi_sendbyte(strobe);
}

//Получить статус
unsigned char cc1101_status(unsigned char strobe){
	spi_sendbyte(strobe | 0xC0);
	return spi_readbyte();
}

//Добавить данные в fifo
void cc1101_white_fifo(unsigned char * data, unsigned char lenght){
	spi_sendbyte(FIFO_TX_BURST);
	spi_sendbyte(lenght + 1);
	spi_sendbyte(CC1101_ADDR);
	for (unsigned char i=0; i<lenght; i++){
		spi_sendbyte(data[i]);
	}
}

/***************************************************************
 *  SmartRF Studio(tm) Export
***************************************************************/
void cc1101_init(){
    cc1101_write_reg(IOCFG0, 0x06);
    cc1101_write_reg(FIFOTHR, 0x47);
    cc1101_write_reg(SYNC1, 0x7A);
    cc1101_write_reg(SYNC0, 0x0E);
    cc1101_write_reg(PKTLEN, 0x14);
    cc1101_write_reg(PKTCTRL0, 0x45);
    cc1101_write_reg(FSCTRL1, 0x06);
    cc1101_write_reg(FREQ2, 0x21);
    cc1101_write_reg(FREQ1, 0x62);
    cc1101_write_reg(FREQ0, 0x76);
    cc1101_write_reg(MDMCFG4, 0xCA);
    cc1101_write_reg(MDMCFG3, 0xF8);
    cc1101_write_reg(MDMCFG2, 0x1E);
    cc1101_write_reg(MDMCFG1, 0x42);
    cc1101_write_reg(DEVIATN, 0x40);
    cc1101_write_reg(MCSM0, 0x18);
    cc1101_write_reg(FOCCFG, 0x16);
    cc1101_write_reg(AGCCTRL2, 0x43);
    cc1101_write_reg(AGCCTRL1, 0x49);
    cc1101_write_reg(WORCTRL, 0xFB);
    cc1101_write_reg(FSCAL3, 0xE9);
    cc1101_write_reg(FSCAL2, 0x2A);
    cc1101_write_reg(FSCAL1, 0x00);
    cc1101_write_reg(FSCAL0, 0x1F);
    cc1101_write_reg(TEST2, 0x81);
    cc1101_write_reg(TEST1, 0x35);
    cc1101_write_reg(TEST0, 0x09);
}

//Отправить пакет
void cc1101_pakage(unsigned char *data, unsigned char lenght){
	spi_start();	
					
	cc1101_white_fifo(data, lenght);
	spi_stop();
				
	_delay_us(10);
				
	spi_start();
	cc1101_strobe(STX);
	cc1101_strobe(SPWD);
	spi_stop();
}
