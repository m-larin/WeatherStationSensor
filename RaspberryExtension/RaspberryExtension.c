/*
 * RaspberryExtensionProject.c
 *
 * Created: 07.08.2016 12:40:11
 *  Author: Михаил
 */ 
#define F_CPU	7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "..\..\Lib\usart\usart.h"

//Порт датчика DHT-22
#define DHT_PORT		PORTC // порт
#define DHT_DDR			DDRC
#define DHT_PIN			PINC
#define DHT_BIT			0 // БИТ порта

//Прорт радиоприемника 
#define TUNER_PORT		PORTC // порт
#define TUNER_DDR		DDRC
#define TUNER_PIN		PINC
#define TUNER_BIT		1 // БИТ порта

//Порт для включения отладочного режима
#define DEBUG_IN_BIT		4 // пин0
#define DEBUG_IN_PORT		PORTB
#define DEBUG_IN_DDR		DDRB
#define DEBUG_IN_PIN		PINB

//Настройка USART
#define BAUD 9600
#define UBRR_VAL F_CPU/16/BAUD-1

//Идентификаторы радиодатчиков
#define RADIO_1_SENSOR_ID	196
#define RADIO_2_SENSOR_ID	206

//Количество 9 секундных отрезков 9 ~= 65536 * 1024 / F_CPU
#define TIMER_COUNT			66 //10 минут
#define TIMER_COUNT_DEBUG	1 //9 секунд

//Данные считываемые с датчика DHT
unsigned char dataDht[5];
//Данные считываемые с радио датчика
unsigned char dataTuner[7];

//Идентификатор пакета от безпроводного датчика, так как безпроводных датчиков 2 то и переменных две
unsigned char batch1Id;
unsigned char batch2Id;

//Число переполнений счетчика
unsigned char overflowCount;

void debug(char *message, int arg0, int arg1, int arg2, int arg3){
	if (!(DEBUG_IN_PIN & _BV(DEBUG_IN_BIT))){
		//Отладочная печать в консоль
		//Признак отладки
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
	//Признак данных
	usart_send(flag);
	for (unsigned char i = 0; i < size; i++)
	{
		usart_send(data[i]);
	}
	usart_send_str("\n\r");
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
	dataDht[0] = dataDht[1] = dataDht[2] = dataDht[3] = dataDht[4] = 0;

	DHT_DDR|=_BV(DHT_BIT); //пин на выход
	DHT_PORT&=~(_BV(DHT_BIT)); //на выходе 0
	_delay_ms(18); //спим 18 мс	, увеличил до 20, не срабатывала модель в протеусе. Самое тонкое место надо бодбирать на МК
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
				dataDht[i] |= (1<<j);
			}
		}
	}
	
	//Проверяем контрольную сумму
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
	//Проверяем принимали ранее эту пачку
	if (*savedBatchId == ressivedBatchId)
	{
		return 1;
	}
	else
	{
		//Если не принимали то запоминаем номер пачки для конкретного сенсора
		*savedBatchId = ressivedBatchId;
		return 0;
	}
}

/*
* Чтение данных из безпроводных датчиков
*/
int readTunerData()
{
	unsigned char sensorId;
	//double temperature, humidity;
	dataTuner[0] = dataTuner[1] = dataTuner[2] = dataTuner[3] = dataTuner[4] = dataTuner[5] = dataTuner[6] = 0;
	
	//Возможно начата передача	
	if (TUNER_PIN&(_BV(TUNER_BIT)))
	{
		int countSupporting;
		//Определяем что это стартовая последовательность
		//Ждем пока передатчик держит HIGH, должен 600 сек, пока не засекаем время
		countSupporting = 0;
		while(TUNER_PIN&(_BV(TUNER_BIT))){
			 _delay_us(10);
			 countSupporting++;
		}
		
		//Если отличается от эталона более чем на 200 мкс то игнорируем
		if (countSupporting < 70 || countSupporting > 90) return 0;

		//Передатчик прекратил передачу, Далее следуют пять последовательностей LOW HIGH по 800 мкс
		int i, j;
		int countLow, countHigh, startCount;
		startCount = 0;
		for (i=0; i<5; i++)
		{
			//Сначала замеряем низкий уровень
			countLow = 0;
			while(!(TUNER_PIN&(_BV(TUNER_BIT))))
			{
				_delay_us(10);
				countLow++;
			}

			//Если отличается от эталона более чем на 100 мкс то игнорируем
			if (countLow < 70 || countLow > 90) return 0;

			//Замеряем высокий уровень
			countHigh = 0;
			while(TUNER_PIN&(_BV(TUNER_BIT)))
			{
				_delay_us(10);
				countHigh++;
			}
			//Если отличается от эталона более чем на 100 мкс то игнорируем эталон равен 80 для 0 и 240 для 1
			if (countHigh < 70 || countHigh > 90) return 0;
			
			//Отладочная печать в консоль
			//debug("Resive start candidate", startCount, countLow, countHigh, 0);

			startCount++;
		}
		//usart_send_str("Start detect\n\r");
		
		//Данные начались, принимаем 48 бит - 6 байт
		//i-номер байта
		for(i=0; i<7; i++)
		{
			//j-номер бита, так как биты идут задом наперед от старшего разряда к младшему цикл строим на уменьшение j от 7 до 0
			for(j=7; j>=0; j--){
				//замеряем на сколько датчик прижал шину к земле, должен на 50 мкс но в условиях без кварца на время надеятся не будем
				//замеряем количество циклов по 1 мкс, при последующем передаче 0 будет меньше а при передаче 1 больше этих циклов
				//все это делаем чтобы исключить влияние температуры на частоту контроллера без кварца
				
				int preDataCount, dataCount;
				preDataCount = 0;
				dataCount = 0;				

				while (!(TUNER_PIN&(_BV(TUNER_BIT)))){
					_delay_us(10);
					preDataCount++;
				}
				
				//Начался бит данных
				while (TUNER_PIN&(_BV(TUNER_BIT))){
					_delay_us(10);
					dataCount++;
				}
				
				//int sub = preDataCount - dataCount;
				//fprintf(pFile, "data test\t%d\t%d\t%d\n", preDataCount, dataCount, sub);
				
				//И самое интересное, если логический ноль на шине был короче чем логическая еденица то приняли байт = 1 иначе 0
				//Пишем только еденицы, так как в начале функции во все байты данных прописали нули
				if (preDataCount < dataCount){
					//Считали 1
					dataTuner[i] |= (1<<j);
				}
			}
		}
			
		//Проверяем контрольную сумму
		unsigned char checkSumm = dataTuner[0] + dataTuner[1] + dataTuner[2] + dataTuner[3] + dataTuner[4] + dataTuner[5];
		if (checkSumm == dataTuner[6])
		{
			//debug("Radio check summ is correct", data[0], data[1], data[2], data[3]);

			sensorId = dataTuner[0];
			//Проверяем идентификатор датчика и может уже принимали эту пачку
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
			
			//Дешифруем данные с радиодатчиков
			/*unsigned int hum=0, tmp=0;
			char sig = 1;
			hum = data[2]<<8;
			hum |= data[3];

			tmp = data[4]<<8;
			//Определяем знак
			if (tmp & (1<<15)){
				sig = 0;
			}
			tmp &= ~(1<<15); //Обнуляем разряд знака температуры
			tmp |= data[5];
			
			//печатаем в uart			
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
	//Порт радио датчика на вход
	TUNER_DDR &= ~(_BV(TUNER_BIT));  
	//Делитель таймера 1 = 1024
	TCCR1B |= _BV(CS12)|_BV(CS10); 
	//Прерывание по переполнению таймера 1
	TIMSK |= _BV(TOIE1);
	//Порт включения отладки на вход
	DEBUG_IN_DDR &= ~(_BV(DEBUG_IN_BIT));
	//Подтягиваем порт включения отладки к 1
	DEBUG_IN_PORT |=_BV(DEBUG_IN_BIT);
	
	//разрешаем глобальные прерывания
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
