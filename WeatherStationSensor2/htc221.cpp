/*
 * htc221.cpp
 *
 * Created: 11.12.2018 22:30:18
 *  Author: Mikhail
 */ 
#include <avr/io.h>
#include "i2c.h"
#include "htc221.h"

float hts221_calculate_aggv(unsigned int y0, unsigned int y1, int x0, int x1, int x){
	return (((float)(x - x0) * (float)(y1 - y0)) / (float)(x1 - x0)) + (float)y0;
}

void hts221_write_reg(unsigned char addreass, unsigned char data)
{
	i2c_start();
	i2c_send_byte(HTS221_WRITE);
	i2c_send_byte(addreass);
	i2c_send_byte(data);
	i2c_stop();
}

void hts221_read_reg(unsigned char addreass, unsigned char * result, unsigned char size)
{
	i2c_start();
	i2c_send_byte(HTS221_WRITE);
	
	// В случае одного байта устанавливаем адрес как в даташите,
	// для чтения нескольких байт надо включить автоинкремент адреса путем добавления седьмого бита
	if (size == 1)
	i2c_send_byte(addreass);
	else
	i2c_send_byte(addreass | 0x80);
	
	i2c_restart();
	i2c_send_byte(HTS221_READ);
	unsigned char i;
	for (i=0; i<size; i++)
	{
		unsigned char lastByte = 0;
		if ((i+1) == size) lastByte = 1;
		result[i] = i2c_get_byte(lastByte);
	}
	i2c_stop();
}

/************************************************************************/
/* Инициализация датчика. В случае успеха вернет 1, иначе 0				*/
/************************************************************************/
unsigned char hts221_init(calibration_data *calibration)
{
	//Проверка что датчик отвечает корректным значением регистра WHO_AM_I
	unsigned char whoAmI;
	hts221_read_reg(0x0F, &whoAmI, 1);
	if (whoAmI != 0xBC)
		return 0;
	
	//Загружаем колибровочные регистры
	unsigned char calbrationRegistres[16];
	hts221_read_reg(0x30, calbrationRegistres, 16);

	//Заполняем переменные
	calibration->H0 = calbrationRegistres[0];
	calibration->H1 = calbrationRegistres[1];
	calibration->T0 = (calbrationRegistres[5] & 0x03)<<8;
	calibration->T0 |=calbrationRegistres[2];
	calibration->T1 = (calbrationRegistres[5] & 0x0C)<<6;
	calibration->T1 |=calbrationRegistres[3];
	calibration->H0_T0_OUT = (calbrationRegistres[7] << 8) | calbrationRegistres[6];
	calibration->H1_T0_OUT = (calbrationRegistres[11] << 8) | calbrationRegistres[10];
	calibration->T0_OUT = (calbrationRegistres[13] << 8) | calbrationRegistres[12];
	calibration->T1_OUT = (calbrationRegistres[15] << 8) | calbrationRegistres[14];
	
	return 1;
}

/*************************************************************************/
/* Чтение влажности */
/************************************************************************/
float hts221_read_humidity(calibration_data calibration)
{
	// Прверка статуса
	unsigned char status=0;
	do{
		hts221_read_reg(0x27, &status, 1);
	}while((status & _BV(1)) == 0x00);
	
	//Загружаем данные влажности
	unsigned char data[2];
	hts221_read_reg(0x28, data, 2);
	
	int h_out = data[1]<<8 | data[0];
	float h = hts221_calculate_aggv(calibration.H0, calibration.H1, calibration.H0_T0_OUT, calibration.H1_T0_OUT, h_out);
	return h / 2;
}

/*************************************************************************/
/* Чтение температуры */
/************************************************************************/
float hts221_read_temperature(calibration_data calibration)
{
	// Прверка статуса
	unsigned char status=0;
	do{
		hts221_read_reg(0x27, &status, 1);
	}while((status & _BV(0)) == 0x00);
	
	//Загружаем данные влажности
	unsigned char data[2];
	hts221_read_reg(0x2A, data, 2);
	
	int t_out = data[1]<<8 | data[0];
	float t = hts221_calculate_aggv(calibration.T0, calibration.T1, calibration.T0_OUT, calibration.T1_OUT, t_out);
	return t / 8;
}
