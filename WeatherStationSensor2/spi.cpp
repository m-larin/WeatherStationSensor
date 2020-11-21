/*
 * CPPFile1.cpp
 *
 * Created: 11.12.2018 1:05:44
 *  Author: Mikhail
 */ 
#include <util/delay.h>
#include <avr/io.h>
#include "spi.h"

//�������������
void spi_init()
{
	//SPI_SS �� �����
	SPI_DDR |= _BV(SPI_SS);
	//MOSI �� �����
	SPI_DDR |= _BV(SPI_MOSI);
	//SCK �� �����
	SPI_DDR |= _BV(SPI_SCK);
	//MISO �� ����
	SPI_DDR &= ~(_BV(SPI_MISO));
	
	//�� SS 1
	SPI_PORT |= _BV(SPI_SS);
	//�� SCK 0
	SPI_PORT &= ~(_BV(SPI_SCK));
	//�� MOSI 0
	SPI_PORT &= ~(_BV(SPI_MOSI));
	//MISO ����������� � 1
	SPI_PORT |= _BV(SPI_MISO);
}

// ����� ������������������
void spi_start(void)
{
	SPI_PORT &= ~(_BV(SPI_SCK));// SPI_MODE = 0
	SPI_PORT &= ~(_BV(SPI_SS)); // Chip Select - Enable
	//���� ������, ���� �� SO �� �������� 0
	while (SPI_PIN & (_BV(SPI_MISO)));
}

// ���� ������������������
void spi_stop(void)
{
	SPI_PORT |= _BV(SPI_SS);// Chip Select - Disable
	SPI_PORT &= ~(_BV(SPI_SCK)); // SPI_MODE = 0
}

// ������� ����
unsigned char spi_sendbyte(unsigned char data)
{
	unsigned char i, status;
	status = 0;
	// ��������� 1 ����
	for(i=0; i<8; i++)
	{
		
		// ��������� ����� ��� = 1
		if (data & 0x80)
		{
			//�������� 1
			SPI_PORT |= _BV(SPI_MOSI);
		}
		else
		{
			//�������� 0
			SPI_PORT &= ~(_BV(SPI_MOSI));
		}
		
		SPI_PORT |= _BV(SPI_SCK); // �������������

		status <<= 1;   // ����� ��� ������ ���� ����
		data <<= 1;     // ����� ��� �������� ���� ����

		if (SPI_PIN & _BV(SPI_MISO)) {
			status |= 0x01; // ������ ���
		}
		//_delay_us(1);
		SPI_PORT &= ~(_BV(SPI_SCK)); // �������������
	}
	_delay_us(50);
	return status;
}

// ��������� ����
unsigned char spi_readbyte(void)
{
	unsigned char i, spiReadData=0;
	
	for(i=0; i<8; i++)
	{
		spiReadData <<= 1; // ����� ��� ������ ���� ����
		
		SPI_PORT |= _BV(SPI_SCK); // �������������
		_delay_us(1);
		if (SPI_PIN & _BV(SPI_MISO)) {
			spiReadData |= 0x01; // ������ ���
		}
		//_delay_us(1);
		SPI_PORT &= ~(_BV(SPI_SCK)); // �������������
	}
	
	return spiReadData;
}
