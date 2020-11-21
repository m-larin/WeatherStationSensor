
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "am2322.h"


/*
* Получение данных с датчика AM2322
*/
int Am2322::read(void)
{
    int dev = wiringPiI2CSetup(AM2322_ADDRESS) ;
    if (dev < 0)
    {
        printf("Error. Devise not init\n");
        return -1;
    }
    
    // Будим устройство
    wiringPiI2CRead(dev);
    delay(1);   
    
    // Формируем запрос
    unsigned char request[2];
    request[0] = 0x00;
    request[1] = 0x04;
    if (wiringPiI2CWriteBlock(dev, 0x03, sizeof(request), request)){
        printf("Error Write Block\n");  
        close(dev);
        return -1;
    }
    delay(2);
    
    // Считываем данные с датчика
    unsigned char response[8];
    memset(response, 0, 8);
    int readCount = wiringPiI2CReadBlock(dev, 0x00, sizeof(response), response);
    int functionCode = response[0];
    int count = response[1];
    int hh = response[2];
    int lh = response[3];
    int ht = response[4];
    int lt = response[5];
    int crcl = response[6];
    int crch = response[7];

    // Расчет контрольной суммы полученных данных
    unsigned short crc = crc16(response, 6);
    
    // Преобразовываем полученную контрольную сумму для сравнения
    unsigned short dcrc=0;  
    dcrc = crch<<8|crcl;
    
    printf("AM2322 %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", readCount, functionCode, count, hh, lh, ht, lt, crcl, crch, crc, dcrc);
    
    // Проверка контрольной суммы
    if (dcrc == crc){
    
        //Дешифруем данные
        unsigned int hum=0, tmp=0;
        char sig = 1;
        hum = hh<<8;
        hum |= lh;

        tmp = ht<<8;
        //Определяем знак
        if (tmp & (1<<15)){
            sig = 0;
        }
        tmp &= ~(1<<15); //Обнуляем разряд знака температуры
        tmp |= lt;
        
        //Переводим в десятичное число
        temperature = (double)tmp / (double)10;
        if (!sig)
        {
            temperature *= (-1);
        }
        humidity = (double)hum / (double)10; 
        close(dev);
        return 0;
    }else{
        printf("AM2322 CRC is incorrect\n");
        close(dev);
        return -1;
    }
}

/*
 * Расчет контрольной суммы для датчика AM2322
 */
unsigned short Am2322::crc16(unsigned char *ptr, unsigned char len)
{
    unsigned short crc =0xFFFF;
    unsigned char i;
    while(len--)
    {
        crc ^=*ptr++;
        for(i=0;i<8;i++)
        {
            if(crc & 0x01)
            {
                crc>>=1;
                crc^=0xA001;
            }
            else
            {
                crc>>=1;
            }
        }
    }
    return crc;
}
