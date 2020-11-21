/*
 * WeatherStationSensor2.c
 *
 * Created: 19.08.2016 16:29:01
 *  Author: larin
 */ 

// Project -> Toolchain -> AVR/GNU C Compiler -> Symbols, add F_CPU=4000000
//#define F_CPU	4000000UL
// Для изменеия уровня батарейки: https://www.rlocman.ru/shem/schematics.html?di=73220

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "cc1101.h"
#include "spi.h"
#include "i2c.h"
#include "htc221.h"

//Количество 8 секундных отрезков между соседними отправками пакетов информации о температуре
#define SLEEP_PERIODS_COUNT			75		//10 минут
//#define SLEEP_PERIODS_COUNT			1		//8 сек для отладки


//ID датчика, случайное число, разное у каждого сенсора, максимум 255
//#define SENSOR_ID		0x11
#define SENSOR_ID		0x12

//Порт для отладки (светодиод)
#define DEBUG_DDR		DDRB
#define DEBUG_BIT		PORTB2
#define DEBUG_PORT		PORTB

/************************************************************************/
/* Функция для отладки, мигаем DEBUG_BIT столько раз сколько передали в параметрах                                                                     */
/************************************************************************/
void debug(unsigned char count){
	for (unsigned char i=0; i<count; i++)
	{
		DEBUG_PORT |= _BV(DEBUG_BIT);
		_delay_ms(200);
		DEBUG_PORT &= ~(_BV(DEBUG_BIT));
		_delay_ms(200);
	}
}

void setup_adc(void)
{
	// Опорное напряжение - питание
	// Измеряемое напряжение - 1.1
	ADMUX = 0x21; 
}


// Получаем напряжение без прерываний, так как надо выполнять измерения последовательно
float get_adc_data()
{
	unsigned int adc_data;
    float vcc;

	// Активируем ЦАП, режим одного измерения
	ADCSRA |= _BV(ADEN);
	_delay_ms(10);
	
	// Запуск измерения
	ADCSRA |= _BV(ADSC);
	
	// Ждем пока ADSC не вернется в 0
	while (ADCSRA & _BV(ADSC));
	
	// читаем 10 bit и вычисляем результат
	adc_data = ADCH << 8 | ADCL; 
	vcc = 1.1 * 1024 / adc_data;

	// Выключаем ЦАП	
	ADCSRA &= ~_BV(ADEN);
	
	return vcc;
}

void send_err(unsigned char err_num){
	unsigned char err[5];
	err[0] = SENSOR_ID;
	err[1] = 'e';
	err[2] = 'r';
	err[3] = 'r';
	err[4] = err_num;
	cc1101_pakage(err, sizeof(err));
}

/***************************************************************/
int main(void)
{
	// Задаем частоту процессора 4Мгц, путем установки делителя в 2?
	// FUSE CKDIV8 должен быть сброшен, чтобы можно было сменить частоту
	CLKPR = _BV(CLKPCE);
	CLKPR = _BV(CLKPS0);
	
	//Включаем watchdog срабатывает каждые 8 секунд
	MCUSR  &=  ~(1<<WDRF);							// Clear WDRF if it has been unintentionally set.
	WDTCSR   =   (1<<WDCE )|(1<<WDE);				// Enable configuration change.
	WDTCSR   =   (1<<WDIF)|(1<<WDIE)|				// Enable Watchdog Interrupt Mode.
		(1<<WDCE )|(0<<WDE  )|						// Disable Watchdog System Reset Mode if unintentionally enabled.
		(1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0);	// Set Watchdog Timeout period to 8.0 sec.
	
	//Разрешаем засыпать в Power-down mode
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	 		
	// разрешаем глобальные прерывания
	sei();	

	// Init ADC
	setup_adc(); 
	
	// Первый запуск чере 8 сек
	unsigned char sleepTime = SLEEP_PERIODS_COUNT-1;
	
	//Порт отладки
	//DEBUG_DDR |= _BV(DEBUG_BIT);
	
	//Инициализация SPI
	spi_init();

	//Проверяем отвечает ли передатчик
	spi_start();
	char version = cc1101_status(VERSION);
	spi_stop();
	if(version == 20){
		//debug(1);
	}else{
		//debug(2);
	}
	
	//Инициализация CC1101
	struct calibration_data calibration;
	spi_start();
	cc1101_strobe(SFTX);
	cc1101_strobe(SRES);
	cc1101_init();
	cc1101_strobe(SPWD);
	spi_stop();

	_delay_ms(10);

	//Инициализация i2С
	i2c_init();

	_delay_ms(10);

	// Инициализация датчика
	if (hts221_init(&calibration)){
		//debug(1);
	}else{
		//debug(2);
		send_err(0x01);
	}

    while(1)
    {
		if (sleepTime >= SLEEP_PERIODS_COUNT){			
			//debug(1);
			// Включаем датчик в режиме одно измерение
			hts221_write_reg(0x20, 0x84);
			// Запускаем измерение
			hts221_write_reg(0x21, 0x01);
			
			//Чтение датчика
			float temperature = hts221_read_temperature(calibration);
			float humidity = hts221_read_humidity(calibration);
						
			// Измеряем заряд батареек
			float vcc = get_adc_data();
			
			//Отправка полученных данных
			unsigned char data[13];	
			data[0] = SENSOR_ID;
			memcpy(&data[1], &temperature, sizeof(temperature));
			memcpy(&data[5], &humidity, sizeof(humidity));
			memcpy(&data[9], &vcc, sizeof(vcc));
			cc1101_pakage(data, sizeof(data));
			sleepTime = 0;
			
			// Делакем задержку, без нее виснет контроллер, так и не понял почему
			_delay_ms(50);
			
			//Выключаем датчик
			hts221_write_reg(0x20, 0x00);			
		}
		
		//Сброс таймера
		wdt_reset();
		//разрешение прерываний от сторожевого таймера (запрещаются автоматически при сбросе сторожевого таймера)
		WDTCSR |= _BV(WDIE);

		//Засыпаем
		sleep_cpu();
		sleepTime++;		
    }
}

//подпрограмма обработки прерывания от сторожевого таймера
ISR(WATCHDOG_vect)
{
	// Запрещаем прерывания
	WDTCSR &= ~(1<<WDIE);	
}