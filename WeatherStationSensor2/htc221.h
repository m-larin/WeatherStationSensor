/*
 * IncFile1.h
 *
 * Created: 11.12.2018 22:29:47
 *  Author: Mikhail
 */ 


#ifndef HTS221_H_
#define HTS221_H_

#define HTS221_READ		0xBF
#define HTS221_WRITE	0xBE

//Колибровочные регистры
struct calibration_data
{	
	unsigned int H0;
	unsigned int H1;
	unsigned int T0;
	unsigned int T1;
	int H0_T0_OUT;
	int H1_T0_OUT;
	int T0_OUT;
	int T1_OUT;
};

float hts221_calculate_aggv(unsigned int y0, unsigned int y1, int x0, int x1, int x);

void hts221_write_reg(unsigned char addreass, unsigned char data);

void hts221_read_reg(unsigned char addreass, unsigned char * result, unsigned char size);

/************************************************************************/
/* Инициализация датчика. В случае успеха вернет 1, иначе 0				*/
/************************************************************************/
unsigned char hts221_init(calibration_data *calibration);

/*************************************************************************/
/* Чтение влажности */
/************************************************************************/
float hts221_read_humidity(calibration_data calibration);

/*************************************************************************/
/* Чтение температуры */
/************************************************************************/
float hts221_read_temperature(calibration_data calibration);

#endif /* HTS221_H_ */