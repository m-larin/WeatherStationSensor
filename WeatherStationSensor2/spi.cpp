/*
 * CPPFile1.cpp
 *
 * Created: 11.12.2018 1:05:44
 *  Author: Mikhail
 */ 
#include <util/delay.h>
#include <avr/io.h>
#include "spi.h"

//Инициализация
void spi_init()
{
	//SPI_SS на выход
	SPI_DDR |= _BV(SPI_SS);
	//MOSI на выход
	SPI_DDR |= _BV(SPI_MOSI);
	//SCK на выход
	SPI_DDR |= _BV(SPI_SCK);
	//MISO на вход
	SPI_DDR &= ~(_BV(SPI_MISO));
	
	//На SS 1
	SPI_PORT |= _BV(SPI_SS);
	//На SCK 0
	SPI_PORT &= ~(_BV(SPI_SCK));
	//На MOSI 0
	SPI_PORT &= ~(_BV(SPI_MOSI));
	//MISO подтягиваем к 1
	SPI_PORT |= _BV(SPI_MISO);
}

// СТАРТ последовательность
void spi_start(void)
{
	SPI_PORT &= ~(_BV(SPI_SCK));// SPI_MODE = 0
	SPI_PORT &= ~(_BV(SPI_SS)); // Chip Select - Enable
	//Ждем ответа, пока на SO не появится 0
	while (SPI_PIN & (_BV(SPI_MISO)));
}

// СТОП последовательность
void spi_stop(void)
{
	SPI_PORT |= _BV(SPI_SS);// Chip Select - Disable
	SPI_PORT &= ~(_BV(SPI_SCK)); // SPI_MODE = 0
}

// ПОСЛАТЬ байт
unsigned char spi_sendbyte(unsigned char data)
{
	unsigned char i, status;
	status = 0;
	// отправить 1 байт
	for(i=0; i<8; i++)
	{
		
		// проверить старш бит = 1
		if (data & 0x80)
		{
			//Передать 1
			SPI_PORT |= _BV(SPI_MOSI);
		}
		else
		{
			//Передать 0
			SPI_PORT &= ~(_BV(SPI_MOSI));
		}
		
		SPI_PORT |= _BV(SPI_SCK); // синхроимпульс

		status <<= 1;   // сдвиг для чтения след бита
		data <<= 1;     // сдвиг для передачи след бита

		if (SPI_PIN & _BV(SPI_MISO)) {
			status |= 0x01; // читаем бит
		}
		//_delay_us(1);
		SPI_PORT &= ~(_BV(SPI_SCK)); // синхроимпульс
	}
	_delay_us(50);
	return status;
}

// ПРОЧИТАТЬ байт
unsigned char spi_readbyte(void)
{
	unsigned char i, spiReadData=0;
	
	for(i=0; i<8; i++)
	{
		spiReadData <<= 1; // сдвиг для чтения след бита
		
		SPI_PORT |= _BV(SPI_SCK); // синхроимпульс
		_delay_us(1);
		if (SPI_PIN & _BV(SPI_MISO)) {
			spiReadData |= 0x01; // читаем бит
		}
		//_delay_us(1);
		SPI_PORT &= ~(_BV(SPI_SCK)); // синхроимпульс
	}
	
	return spiReadData;
}
