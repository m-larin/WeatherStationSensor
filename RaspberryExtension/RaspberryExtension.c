/*
 * RaspberryExtensionProject.c
 *
 * Created: 07.08.2016 12:40:11
 *  Author: ������
 */ 
#define F_CPU	7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "..\..\Lib\usart\usart.h"

//���� ������� DHT-22
#define DHT_PORT		PORTC // ����
#define DHT_DDR			DDRC
#define DHT_PIN			PINC
#define DHT_BIT			0 // ��� �����

//����� �������������� 
#define TUNER_PORT		PORTC // ����
#define TUNER_DDR		DDRC
#define TUNER_PIN		PINC
#define TUNER_BIT		1 // ��� �����

//���� ��� ��������� ����������� ������
#define DEBUG_IN_BIT		4 // ���0
#define DEBUG_IN_PORT		PORTB
#define DEBUG_IN_DDR		DDRB
#define DEBUG_IN_PIN		PINB

//��������� USART
#define BAUD 9600
#define UBRR_VAL F_CPU/16/BAUD-1

//�������������� �������������
#define RADIO_1_SENSOR_ID	196
#define RADIO_2_SENSOR_ID	206

//���������� 9 ��������� �������� 9 ~= 65536 * 1024 / F_CPU
#define TIMER_COUNT			66 //10 �����
#define TIMER_COUNT_DEBUG	1 //9 ������

//������ ����������� � ������� DHT
unsigned char dataDht[5];
//������ ����������� � ����� �������
unsigned char dataTuner[7];

//������������� ������ �� ������������� �������, ��� ��� ������������ �������� 2 �� � ���������� ���
unsigned char batch1Id;
unsigned char batch2Id;

//����� ������������ ��������
unsigned char overflowCount;

void debug(char *message, int arg0, int arg1, int arg2, int arg3){
	if (!(DEBUG_IN_PIN & _BV(DEBUG_IN_BIT))){
		//���������� ������ � �������
		//������� �������
		usart_send('I');
		usart_send_str(message);
		usart_send_str(" ");
		usart_send_int(arg0);
		usart_send_str(" ");
		usart_send_int(arg1);
		usart_send_str(" ");
		usart_send_int(arg2);
		usart_send_str(" ");
		usart_send_int(arg3);
		usart_send_str("\n\r");	
	}
}

void sendData(unsigned char flag, unsigned char *data, unsigned char size){
	//������� ������
	usart_send(flag);
	for (unsigned char i = 0; i < size; i++)
	{
		usart_send(data[i]);
	}
	usart_send_str("\n\r");
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
	dataDht[0] = dataDht[1] = dataDht[2] = dataDht[3] = dataDht[4] = 0;

	DHT_DDR|=_BV(DHT_BIT); //��� �� �����
	DHT_PORT&=~(_BV(DHT_BIT)); //�� ������ 0
	_delay_ms(18); //���� 18 ��	, �������� �� 20, �� ����������� ������ � ��������. ����� ������ ����� ���� ��������� �� ��
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
				dataDht[i] |= (1<<j);
			}
		}
	}
	
	//��������� ����������� �����
	unsigned char checkSumm = dataDht[0] + dataDht[1] + dataDht[2] + dataDht[3];
	if (checkSumm == dataDht[4])
	{
		debug("DHT OK", dataDht[0], dataDht[1], dataDht[2], dataDht[3]);
		return 1;
	}
	else
	{
		debug("DHT Check summ filed", 0, 0, 0, 0);
		return 0;
	}
}

int isReceiveBatch(unsigned char* savedBatchId, unsigned char ressivedBatchId)
{
	//��������� ��������� ����� ��� �����
	if (*savedBatchId == ressivedBatchId)
	{
		return 1;
	}
	else
	{
		//���� �� ��������� �� ���������� ����� ����� ��� ����������� �������
		*savedBatchId = ressivedBatchId;
		return 0;
	}
}

/*
* ������ ������ �� ������������ ��������
*/
int readTunerData()
{
	unsigned char sensorId;
	//double temperature, humidity;
	dataTuner[0] = dataTuner[1] = dataTuner[2] = dataTuner[3] = dataTuner[4] = dataTuner[5] = dataTuner[6] = 0;
	
	//�������� ������ ��������	
	if (TUNER_PIN&(_BV(TUNER_BIT)))
	{
		int countSupporting;
		//���������� ��� ��� ��������� ������������������
		//���� ���� ���������� ������ HIGH, ������ 600 ���, ���� �� �������� �����
		countSupporting = 0;
		while(TUNER_PIN&(_BV(TUNER_BIT))){
			 _delay_us(10);
			 countSupporting++;
		}
		
		//���� ���������� �� ������� ����� ��� �� 200 ��� �� ����������
		if (countSupporting < 70 || countSupporting > 90) return 0;

		//���������� ��������� ��������, ����� ������� ���� ������������������� LOW HIGH �� 800 ���
		int i, j;
		int countLow, countHigh, startCount;
		startCount = 0;
		for (i=0; i<5; i++)
		{
			//������� �������� ������ �������
			countLow = 0;
			while(!(TUNER_PIN&(_BV(TUNER_BIT))))
			{
				_delay_us(10);
				countLow++;
			}

			//���� ���������� �� ������� ����� ��� �� 100 ��� �� ����������
			if (countLow < 70 || countLow > 90) return 0;

			//�������� ������� �������
			countHigh = 0;
			while(TUNER_PIN&(_BV(TUNER_BIT)))
			{
				_delay_us(10);
				countHigh++;
			}
			//���� ���������� �� ������� ����� ��� �� 100 ��� �� ���������� ������ ����� 80 ��� 0 � 240 ��� 1
			if (countHigh < 70 || countHigh > 90) return 0;
			
			//���������� ������ � �������
			//debug("Resive start candidate", startCount, countLow, countHigh, 0);

			startCount++;
		}
		//usart_send_str("Start detect\n\r");
		
		//������ ��������, ��������� 48 ��� - 6 ����
		//i-����� �����
		for(i=0; i<7; i++)
		{
			//j-����� ����, ��� ��� ���� ���� ����� ������� �� �������� ������� � �������� ���� ������ �� ���������� j �� 7 �� 0
			for(j=7; j>=0; j--){
				//�������� �� ������� ������ ������ ���� � �����, ������ �� 50 ��� �� � �������� ��� ������ �� ����� �������� �� �����
				//�������� ���������� ������ �� 1 ���, ��� ����������� �������� 0 ����� ������ � ��� �������� 1 ������ ���� ������
				//��� ��� ������ ����� ��������� ������� ����������� �� ������� ����������� ��� ������
				
				int preDataCount, dataCount;
				preDataCount = 0;
				dataCount = 0;				

				while (!(TUNER_PIN&(_BV(TUNER_BIT)))){
					_delay_us(10);
					preDataCount++;
				}
				
				//������� ��� ������
				while (TUNER_PIN&(_BV(TUNER_BIT))){
					_delay_us(10);
					dataCount++;
				}
				
				//int sub = preDataCount - dataCount;
				//fprintf(pFile, "data test\t%d\t%d\t%d\n", preDataCount, dataCount, sub);
				
				//� ����� ����������, ���� ���������� ���� �� ���� ��� ������ ��� ���������� ������� �� ������� ���� = 1 ����� 0
				//����� ������ �������, ��� ��� � ������ ������� �� ��� ����� ������ ��������� ����
				if (preDataCount < dataCount){
					//������� 1
					dataTuner[i] |= (1<<j);
				}
			}
		}
			
		//��������� ����������� �����
		unsigned char checkSumm = dataTuner[0] + dataTuner[1] + dataTuner[2] + dataTuner[3] + dataTuner[4] + dataTuner[5];
		if (checkSumm == dataTuner[6])
		{
			//debug("Radio check summ is correct", data[0], data[1], data[2], data[3]);

			sensorId = dataTuner[0];
			//��������� ������������� ������� � ����� ��� ��������� ��� �����
			unsigned char receivedBatchId = dataTuner[1];
			if (sensorId == RADIO_1_SENSOR_ID)
			{
				if (isReceiveBatch(&batch1Id, receivedBatchId))
				{
					return 0;
				}
			}
			else if(sensorId == RADIO_2_SENSOR_ID)
			{
				if (isReceiveBatch(&batch2Id, receivedBatchId))
				{
					return 0;
				}
			}else{
				return 0;
			}
			
			//��������� ������ � �������������
			/*unsigned int hum=0, tmp=0;
			char sig = 1;
			hum = data[2]<<8;
			hum |= data[3];

			tmp = data[4]<<8;
			//���������� ����
			if (tmp & (1<<15)){
				sig = 0;
			}
			tmp &= ~(1<<15); //�������� ������ ����� �����������
			tmp |= data[5];
			
			//�������� � uart			
			usart_send_str("SensorId=");
			usart_send_int(sensorId);
			usart_send_str(" BatchId=");
			usart_send_int(receivedBatchId);
			usart_send_str(" Humidity=");
			usart_send_int(hum);
			usart_send_str(" Temperature=");
			if (!sig){
				usart_send('-');
			}
			usart_send_int(tmp);
			usart_send_str("\n\r");*/
			debug("Tuner OK", dataTuner[0], dataTuner[1], dataTuner[2], dataTuner[3]);
			return 1;
		}
		else
		{
			debug("Tuner check summ is incorrect", 0, 0, 0, 0);
			return 0;
		}
		
	}else{
		return 0;
	}
}


int main(void)
{
	usart_init(UBRR_VAL);
	//���� ����� ������� �� ����
	TUNER_DDR &= ~(_BV(TUNER_BIT));  
	//�������� ������� 1 = 1024
	TCCR1B |= _BV(CS12)|_BV(CS10); 
	//���������� �� ������������ ������� 1
	TIMSK |= _BV(TOIE1);
	//���� ��������� ������� �� ����
	DEBUG_IN_DDR &= ~(_BV(DEBUG_IN_BIT));
	//����������� ���� ��������� ������� � 1
	DEBUG_IN_PORT |=_BV(DEBUG_IN_BIT);
	
	//��������� ���������� ����������
	sei();	
	debug("Start work", 0, 0, 0, 0);
	
    while(1)
    {
		if (readTunerData()){
			sendData('T', dataTuner, 7);
		}
		_delay_us(50);	
    }
}


ISR(TIMER1_OVF_vect){
	if ((overflowCount >= TIMER_COUNT && (DEBUG_IN_PIN & _BV(DEBUG_IN_BIT)))
		|| (overflowCount >= TIMER_COUNT_DEBUG && !(DEBUG_IN_PIN & _BV(DEBUG_IN_BIT))))
		{	
			overflowCount = 0;
			if (dhtread()){
				sendData('D', dataDht, 5);
			}
		}else{
			overflowCount++;
		}
}
