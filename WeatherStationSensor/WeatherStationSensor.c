/*
* WeatherStationSensor.cpp
*
* Created: 28.07.2015 10:54:06
*  Author: larin
*/

#define F_CPU	4800000UL

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include "..\..\Lib\usart\usart.h"

//Порт датчика
#define DHT_PORT		PORTB // порт
#define DHT_DDR			DDRB
#define DHT_PIN			PINB
#define DHT_BIT			4 // БИТ порта

//Порт передатчика
#define OUT_BIT			2 // пин 2
#define OUT_PORT		PORTB
#define OUT_DDR			DDRB

//Порт питания датчика и передатчика
#define PWR_BIT			3 // пин3
#define PWR_PORT		PORTB
#define PWR_DDR			DDRB

//Порт для отладки (светодиод)
#define DEBUG_BIT		1 // пин1
#define DEBUG_PORT		PORTB
#define DEBUG_DDR		DDRB

//Порт для включения отладочного режима
#define DEBUG_IN_BIT		0 // пин0
#define DEBUG_IN_PORT		PORTB
#define DEBUG_IN_DDR		DDRB
#define DEBUG_IN_PIN		PINB

//Случайное число, разное у каждого сенсора, максимум 255
#define SENSOR_ID		196 
//#define SENSOR_ID		206 

#define OUT_UP			OUT_PORT |= _BV(OUT_BIT);
#define OUT_DOWN		OUT_PORT &= ~(_BV(OUT_BIT));

#define PWR_UP			PWR_PORT |= _BV(PWR_BIT);
#define PWR_DOWN		PWR_PORT &= ~(_BV(PWR_BIT));

//Количество 8 секундных отрезков между соседними отправками пакетов информации о температуре
#define SLEEP_PERIODS_COUNT			75		//10 минут
#define SLEEP_PERIODS_COUNT_DEBUG	1		//8 сек

//Количество отправленных одинаковых посылок в течение одного пакета
#define SEND_COUNT			20
//Количество стартовых импульсов
#define START_IMPULSE_COUNT	5
//Период импульсов передатчика. На выходе будут комбинации из 1.2,3 итд периодов
#define PERIODUSEC_1			400
#define PERIODUSEC_2			PERIODUSEC_1 * 2
#define PERIODUSEC_3			PERIODUSEC_1 * 3
#define PERIODUSEC_30			PERIODUSEC_1 * 30


//макросы вычисления скорости
//#define BAUD 9600
//#define UBRR_VAL F_CPU/16/BAUD-1

//Данные считываемые с датчика
unsigned char datadht[5];

//Идентификатор пакета
unsigned char batchId = 0;

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

/*************************************************************************
1. Контроллер переводит свой пин в режим выхода - как минимум на 18 миллисекунд переводит его в LOW, затем на 40 мкс в HIGH - и, после этого, переключает его в режим входа;
2. Через 20..40 мкс DHT11 отвечает подтверждением, также сначала переводя шину в LOW на 80 мкс, а затем в HIGH на 80 мкс;
3. DHT11 начинает побитную передачу информации, каждый бит начинается с уровня LOW в течение 50 мкс, и затем HIGH разной продолжительности: если 26-28 мкс, то это ноль, если же 70 мкс - единица;
4. По окончании передачи 40 бит информации DHT11 "на прощание" еще раз переводит шину в LOW на 50 мкс и освобождает ее.
Если в программе имеются прерывания,то не забывайте их отлючать перед чтением датчика     */
/************************************************************************/
int dhtread()
{
	datadht[0] = datadht[1] = datadht[2] = datadht[3] = datadht[4] = 0;

	DHT_DDR|=_BV(DHT_BIT); //пин на выход
	DHT_PORT&=~(_BV(DHT_BIT)); //на выходе 0
	_delay_ms(18); //спим 18 мс	, увеличил до 20, не срабатывала модель в протеусе. Самое тонкое место ад бодбирать на МК
	DHT_PORT|=(_BV(DHT_BIT)); //Отпускаем линию
	DHT_DDR&=~(_BV(DHT_BIT));  //Порт на вход

	//Ждем ответа дат+чика - 0 на шине
	unsigned char count = 0;
	while (DHT_PIN&(_BV(DHT_BIT))){
		//Датчик должен ответить через 20-40 мкс, но мы ждем 200 мкс, для надежности
		//Пока на шине 1 крутимся в цикле, но не более 200 мкс, если вышли за 200 мкс то датчик уже не ответит
		_delay_us(5);
		count++;
		if (count > 40){ 
			return 0;
		}
	}
	
	//Датчик ответил, теперь ждем пока он отпустить шину через 80 мкс
	//Крутимся в цикле пока на шине 0
	while (!(DHT_PIN&(_BV(DHT_BIT))));

	//теперь ждем пока начнется передача данных
	//Крутимся в цикле пока на шине 1
	while (DHT_PIN&(_BV(DHT_BIT)));
	
	//Данные начались, принимаем 40 бит - 5 байт
	//i-номер байта
	for(int i=0; i<5; i++){
		//j-номер бита, так как биты идут задом наперед от старшего разряда к младшему цикл строим на уменьшение j от 7 до 0
		for(int j=7; j>=0; j--){
			//замеряем на сколько датчик прижал шину к земле, должен на 50 мкс но в условиях без кварца на время надеятся не будем
			//замеряем количество циклов по 1 мкс, при последующем передаче 0 будет меньше а при передаче 1 больше этих циклов
			//все это делаем чтобы исключить влияние температуры на частоту контроллера без кварца
			int preDataCount = 0;
			while (!(DHT_PIN&(_BV(DHT_BIT)))){
				_delay_us(1);
				preDataCount++;
			}
			
			//Начался бит данных
			int dataCount = 0;
			while (DHT_PIN&(_BV(DHT_BIT))){
				_delay_us(1);
				dataCount++;
			}
			
			//И самое интересное, если логический ноль на шине был короче чем логическая еденица то приняли байт = 1 иначе 0
			//Пишем только еденицы, так как в начале функции во все байты данных прописали нули
			if (preDataCount < dataCount){
				//Считали 1
				datadht[i] |= (1<<j);
			}
		}
	}
	
	//sendSensorDataToUart();
	
	//Проверяем контрольную сумму
	unsigned char checkSumm = datadht[0] + datadht[1] + datadht[2] + datadht[3];
	if (checkSumm == datadht[4])
	{
		debug(1);
		return 1;
	}
	else
	{
		debug(2);
		return 0;
	}
}

/*void sendSensorDataToUart(){
unsigned char checkSumm = datadht[0] +datadht[1] +datadht[2]+datadht[3];
if (checkSumm == datadht[4]){
send_Uart_str("Check summ is valid. ");
}else{
send_Uart_str("Check summ is NOT valid. ");
}

send_Uart_str("Humidity=");
unsigned int hum=0, tmp=0;
char sig = 1;
hum = datadht[0]<<8;
hum |= datadht[1];

tmp = datadht[2]<<8;
//Определяем знак
if (tmp & (1<<15)){
sig = 0;
}
tmp &= ~(1<<15); //Обнуляем разряд знака температуры
tmp |= datadht[3];

send_int_Uart(hum);
send_Uart_str(" Temperature=");
if (!sig){
send_Uart('-');
}
send_int_Uart(tmp);

send_Uart(13);
}*/

void send1Wiere(unsigned char repetitionCount){
	//Передаем 6 байт
	unsigned char sendData[7];
	//Идентификатор сенсора
	sendData[0] = SENSOR_ID;
	//Идентификатор пакета
	sendData[1] = batchId;
	//Данные температуры и влажности
	sendData[2] = datadht[0];
	sendData[3] = datadht[1];
	sendData[4] = datadht[2];
	sendData[5] = datadht[3];
	//контрольная сумма
	sendData[6] = sendData[0] + sendData[1] + sendData[2] + sendData[3] + sendData[4] + sendData[5];
	
	//Количество повторений при отправке в одном пакете
	for (unsigned char c=0; c<repetitionCount; c++){
		//Стартовые биты 
		//1 высокий на 800 мкс
		OUT_UP;
		_delay_us(PERIODUSEC_2);
		//Затем идет START_IMPULSE_COUNT последовательностей из низкого и высокого уровней по 800 мкс
		for(unsigned char i=0; i<START_IMPULSE_COUNT; i++)
		{
			OUT_DOWN;
			_delay_us(PERIODUSEC_2);
			OUT_UP;
			_delay_us(PERIODUSEC_2);
		}
		
		//отправляем в цикле данные
		for (unsigned char i=0; i<7; i++){
			for (signed char j=7; j>=0; j--){

				//Устанавливаем 0 на 800 мкс, опорный импульс
				OUT_DOWN;
				_delay_us(PERIODUSEC_2);

				//Передаем данные для этого переводим пин в 1 на определенное время в зависисмости от данных
				OUT_UP;
				//1 на 1200 мкс
				//0 на 400 мкс
				if (sendData[i] & _BV(j))
				{
					_delay_us(PERIODUSEC_3);
				}
				else
				{
					_delay_us(PERIODUSEC_1);
				}
			}
		}
		
		//После отправки всей посылки прижимаем к 0 не менее чем на 12 мс и повторяем итерацию
		OUT_DOWN;
		_delay_us(PERIODUSEC_30);		
	}
	batchId++;
	debug(3);
}

int mainTest(void)
{
	//Порт OUT выход
	OUT_DDR |= _BV(OUT_BIT);
	
	int period = 400;
	while(1)
	{
		//На выходе 0
		OUT_DOWN;
		_delay_us(period);
		OUT_UP;
		_delay_us(period);
	}
	
}

int main(void)
{
	//usart_init(UBRR_VAL);
	
	//Порт OUT выход
	OUT_DDR |= _BV(OUT_BIT);
	//На выходе 0
	OUT_DOWN;
	//Порт PWR На выход
	PWR_DDR |= _BV(PWR_BIT);
	//Порт питания на 0
	PWR_DOWN;
	//Порт отладки на выход
	DEBUG_DDR |= _BV(DEBUG_BIT);
	//Порт включения отладки на вход
	DEBUG_IN_DDR &= ~(_BV(DEBUG_IN_BIT));
	//Подтягиваем порт включения отладки к 1
	DEBUG_IN_PORT |= _BV(DEBUG_IN_BIT);

	//Включаем watchdog срабатывает каждые 8 секунд
	MCUSR  &=  ~(1<<WDRF);                                 // Clear WDRF if it has been unintentionally set.
	WDTCR   =   (1<<WDCE )|(1<<WDE);                     // Enable configuration change.
	WDTCR   =   (1<<WDTIF)|(1<<WDTIE)|                     // Enable Watchdog Interrupt Mode.
	(1<<WDCE )|(0<<WDE  )|                     // Disable Watchdog System Reset Mode if unintentionally enabled.
	(1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0); // Set Watchdog Timeout period to 8.0 sec.
	
	//Разрешаем засыпать в Power-down mode
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	//разрешаем глобальные прерывания
	sei();
	
	debug(1);
	unsigned char sleepTime = 0;
	while(1)
	{
		if ((sleepTime >= SLEEP_PERIODS_COUNT && (DEBUG_IN_PIN&(_BV(DEBUG_IN_BIT)))) 
			|| (sleepTime >= SLEEP_PERIODS_COUNT_DEBUG && !(DEBUG_IN_PIN&(_BV(DEBUG_IN_BIT))))){
			//Сбрасываем счетчик засыпаний
			sleepTime = 0;
			//Включаем питание датчика и передатчика
			PWR_UP;
			//Спим 1 секунду, раньше датчик не ответит, в некоторых источниках надо ждать 2 секунды
			_delay_ms(2000);
			//Опрашиваем датчик
			if (dhtread()){
				//Если ответил и сошлась контрольная сумма отправляем в передатчик
				send1Wiere(SEND_COUNT);				
			}else{
				debug(4);
			}
			//Отключаем датчик и передатчик
			PWR_DOWN;
		}		

		//Сброс таймера
		wdt_reset();
		//разрешение прерываний от сторожевого таймера (запрещаются автоматически при сбросе сторожевого таймера)
		WDTCR |= _BV(WDTIE);

		//Засыпаем
		sleep_cpu();
		//В протеусе не работают прерывания по WD поэтому для отладки раскометировать следующую строчку и закоментировать предыдущую
		//_delay_ms(5000);
		sleepTime++;
	}
}

//подпрограмма обработки прерывания от сторожевого таймера
ISR(WDT_vect)
{
	// Disable Watchdog Interrupt Mode
	// This can be omitted if repeated interrups are needed
	WDTCR &= ~(1<<WDTIE);	
}