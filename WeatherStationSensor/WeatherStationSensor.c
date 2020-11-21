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

//���� �������
#define DHT_PORT		PORTB // ����
#define DHT_DDR			DDRB
#define DHT_PIN			PINB
#define DHT_BIT			4 // ��� �����

//���� �����������
#define OUT_BIT			2 // ��� 2
#define OUT_PORT		PORTB
#define OUT_DDR			DDRB

//���� ������� ������� � �����������
#define PWR_BIT			3 // ���3
#define PWR_PORT		PORTB
#define PWR_DDR			DDRB

//���� ��� ������� (���������)
#define DEBUG_BIT		1 // ���1
#define DEBUG_PORT		PORTB
#define DEBUG_DDR		DDRB

//���� ��� ��������� ����������� ������
#define DEBUG_IN_BIT		0 // ���0
#define DEBUG_IN_PORT		PORTB
#define DEBUG_IN_DDR		DDRB
#define DEBUG_IN_PIN		PINB

//��������� �����, ������ � ������� �������, �������� 255
#define SENSOR_ID		196 
//#define SENSOR_ID		206 

#define OUT_UP			OUT_PORT |= _BV(OUT_BIT);
#define OUT_DOWN		OUT_PORT &= ~(_BV(OUT_BIT));

#define PWR_UP			PWR_PORT |= _BV(PWR_BIT);
#define PWR_DOWN		PWR_PORT &= ~(_BV(PWR_BIT));

//���������� 8 ��������� �������� ����� ��������� ���������� ������� ���������� � �����������
#define SLEEP_PERIODS_COUNT			75		//10 �����
#define SLEEP_PERIODS_COUNT_DEBUG	1		//8 ���

//���������� ������������ ���������� ������� � ������� ������ ������
#define SEND_COUNT			20
//���������� ��������� ���������
#define START_IMPULSE_COUNT	5
//������ ��������� �����������. �� ������ ����� ���������� �� 1.2,3 ��� ��������
#define PERIODUSEC_1			400
#define PERIODUSEC_2			PERIODUSEC_1 * 2
#define PERIODUSEC_3			PERIODUSEC_1 * 3
#define PERIODUSEC_30			PERIODUSEC_1 * 30


//������� ���������� ��������
//#define BAUD 9600
//#define UBRR_VAL F_CPU/16/BAUD-1

//������ ����������� � �������
unsigned char datadht[5];

//������������� ������
unsigned char batchId = 0;

/************************************************************************/
/* ������� ��� �������, ������ DEBUG_BIT ������� ��� ������� �������� � ����������                                                                     */
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
1. ���������� ��������� ���� ��� � ����� ������ - ��� ������� �� 18 ����������� ��������� ��� � LOW, ����� �� 40 ��� � HIGH - �, ����� �����, ����������� ��� � ����� �����;
2. ����� 20..40 ��� DHT11 �������� ��������������, ����� ������� �������� ���� � LOW �� 80 ���, � ����� � HIGH �� 80 ���;
3. DHT11 �������� �������� �������� ����������, ������ ��� ���������� � ������ LOW � ������� 50 ���, � ����� HIGH ������ �����������������: ���� 26-28 ���, �� ��� ����, ���� �� 70 ��� - �������;
4. �� ��������� �������� 40 ��� ���������� DHT11 "�� ��������" ��� ��� ��������� ���� � LOW �� 50 ��� � ����������� ��.
���� � ��������� ������� ����������,�� �� ��������� �� �������� ����� ������� �������     */
/************************************************************************/
int dhtread()
{
	datadht[0] = datadht[1] = datadht[2] = datadht[3] = datadht[4] = 0;

	DHT_DDR|=_BV(DHT_BIT); //��� �� �����
	DHT_PORT&=~(_BV(DHT_BIT)); //�� ������ 0
	_delay_ms(18); //���� 18 ��	, �������� �� 20, �� ����������� ������ � ��������. ����� ������ ����� �� ��������� �� ��
	DHT_PORT|=(_BV(DHT_BIT)); //��������� �����
	DHT_DDR&=~(_BV(DHT_BIT));  //���� �� ����

	//���� ������ ���+���� - 0 �� ����
	unsigned char count = 0;
	while (DHT_PIN&(_BV(DHT_BIT))){
		//������ ������ �������� ����� 20-40 ���, �� �� ���� 200 ���, ��� ����������
		//���� �� ���� 1 �������� � �����, �� �� ����� 200 ���, ���� ����� �� 200 ��� �� ������ ��� �� �������
		_delay_us(5);
		count++;
		if (count > 40){ 
			return 0;
		}
	}
	
	//������ �������, ������ ���� ���� �� ��������� ���� ����� 80 ���
	//�������� � ����� ���� �� ���� 0
	while (!(DHT_PIN&(_BV(DHT_BIT))));

	//������ ���� ���� �������� �������� ������
	//�������� � ����� ���� �� ���� 1
	while (DHT_PIN&(_BV(DHT_BIT)));
	
	//������ ��������, ��������� 40 ��� - 5 ����
	//i-����� �����
	for(int i=0; i<5; i++){
		//j-����� ����, ��� ��� ���� ���� ����� ������� �� �������� ������� � �������� ���� ������ �� ���������� j �� 7 �� 0
		for(int j=7; j>=0; j--){
			//�������� �� ������� ������ ������ ���� � �����, ������ �� 50 ��� �� � �������� ��� ������ �� ����� �������� �� �����
			//�������� ���������� ������ �� 1 ���, ��� ����������� �������� 0 ����� ������ � ��� �������� 1 ������ ���� ������
			//��� ��� ������ ����� ��������� ������� ����������� �� ������� ����������� ��� ������
			int preDataCount = 0;
			while (!(DHT_PIN&(_BV(DHT_BIT)))){
				_delay_us(1);
				preDataCount++;
			}
			
			//������� ��� ������
			int dataCount = 0;
			while (DHT_PIN&(_BV(DHT_BIT))){
				_delay_us(1);
				dataCount++;
			}
			
			//� ����� ����������, ���� ���������� ���� �� ���� ��� ������ ��� ���������� ������� �� ������� ���� = 1 ����� 0
			//����� ������ �������, ��� ��� � ������ ������� �� ��� ����� ������ ��������� ����
			if (preDataCount < dataCount){
				//������� 1
				datadht[i] |= (1<<j);
			}
		}
	}
	
	//sendSensorDataToUart();
	
	//��������� ����������� �����
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
//���������� ����
if (tmp & (1<<15)){
sig = 0;
}
tmp &= ~(1<<15); //�������� ������ ����� �����������
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
	//�������� 6 ����
	unsigned char sendData[7];
	//������������� �������
	sendData[0] = SENSOR_ID;
	//������������� ������
	sendData[1] = batchId;
	//������ ����������� � ���������
	sendData[2] = datadht[0];
	sendData[3] = datadht[1];
	sendData[4] = datadht[2];
	sendData[5] = datadht[3];
	//����������� �����
	sendData[6] = sendData[0] + sendData[1] + sendData[2] + sendData[3] + sendData[4] + sendData[5];
	
	//���������� ���������� ��� �������� � ����� ������
	for (unsigned char c=0; c<repetitionCount; c++){
		//��������� ���� 
		//1 ������� �� 800 ���
		OUT_UP;
		_delay_us(PERIODUSEC_2);
		//����� ���� START_IMPULSE_COUNT ������������������� �� ������� � �������� ������� �� 800 ���
		for(unsigned char i=0; i<START_IMPULSE_COUNT; i++)
		{
			OUT_DOWN;
			_delay_us(PERIODUSEC_2);
			OUT_UP;
			_delay_us(PERIODUSEC_2);
		}
		
		//���������� � ����� ������
		for (unsigned char i=0; i<7; i++){
			for (signed char j=7; j>=0; j--){

				//������������� 0 �� 800 ���, ������� �������
				OUT_DOWN;
				_delay_us(PERIODUSEC_2);

				//�������� ������ ��� ����� ��������� ��� � 1 �� ������������ ����� � ������������ �� ������
				OUT_UP;
				//1 �� 1200 ���
				//0 �� 400 ���
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
		
		//����� �������� ���� ������� ��������� � 0 �� ����� ��� �� 12 �� � ��������� ��������
		OUT_DOWN;
		_delay_us(PERIODUSEC_30);		
	}
	batchId++;
	debug(3);
}

int mainTest(void)
{
	//���� OUT �����
	OUT_DDR |= _BV(OUT_BIT);
	
	int period = 400;
	while(1)
	{
		//�� ������ 0
		OUT_DOWN;
		_delay_us(period);
		OUT_UP;
		_delay_us(period);
	}
	
}

int main(void)
{
	//usart_init(UBRR_VAL);
	
	//���� OUT �����
	OUT_DDR |= _BV(OUT_BIT);
	//�� ������ 0
	OUT_DOWN;
	//���� PWR �� �����
	PWR_DDR |= _BV(PWR_BIT);
	//���� ������� �� 0
	PWR_DOWN;
	//���� ������� �� �����
	DEBUG_DDR |= _BV(DEBUG_BIT);
	//���� ��������� ������� �� ����
	DEBUG_IN_DDR &= ~(_BV(DEBUG_IN_BIT));
	//����������� ���� ��������� ������� � 1
	DEBUG_IN_PORT |= _BV(DEBUG_IN_BIT);

	//�������� watchdog ����������� ������ 8 ������
	MCUSR  &=  ~(1<<WDRF);                                 // Clear WDRF if it has been unintentionally set.
	WDTCR   =   (1<<WDCE )|(1<<WDE);                     // Enable configuration change.
	WDTCR   =   (1<<WDTIF)|(1<<WDTIE)|                     // Enable Watchdog Interrupt Mode.
	(1<<WDCE )|(0<<WDE  )|                     // Disable Watchdog System Reset Mode if unintentionally enabled.
	(1<<WDP3 )|(0<<WDP2 )|(0<<WDP1)|(1<<WDP0); // Set Watchdog Timeout period to 8.0 sec.
	
	//��������� �������� � Power-down mode
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	//��������� ���������� ����������
	sei();
	
	debug(1);
	unsigned char sleepTime = 0;
	while(1)
	{
		if ((sleepTime >= SLEEP_PERIODS_COUNT && (DEBUG_IN_PIN&(_BV(DEBUG_IN_BIT)))) 
			|| (sleepTime >= SLEEP_PERIODS_COUNT_DEBUG && !(DEBUG_IN_PIN&(_BV(DEBUG_IN_BIT))))){
			//���������� ������� ���������
			sleepTime = 0;
			//�������� ������� ������� � �����������
			PWR_UP;
			//���� 1 �������, ������ ������ �� �������, � ��������� ���������� ���� ����� 2 �������
			_delay_ms(2000);
			//���������� ������
			if (dhtread()){
				//���� ������� � ������� ����������� ����� ���������� � ����������
				send1Wiere(SEND_COUNT);				
			}else{
				debug(4);
			}
			//��������� ������ � ����������
			PWR_DOWN;
		}		

		//����� �������
		wdt_reset();
		//���������� ���������� �� ����������� ������� (����������� ������������� ��� ������ ����������� �������)
		WDTCR |= _BV(WDTIE);

		//��������
		sleep_cpu();
		//� �������� �� �������� ���������� �� WD ������� ��� ������� ��������������� ��������� ������� � ��������������� ����������
		//_delay_ms(5000);
		sleepTime++;
	}
}

//������������ ��������� ���������� �� ����������� �������
ISR(WDT_vect)
{
	// Disable Watchdog Interrupt Mode
	// This can be omitted if repeated interrups are needed
	WDTCR &= ~(1<<WDTIE);	
}