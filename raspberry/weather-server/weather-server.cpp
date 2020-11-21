#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <mysql/mysql.h>
#include <string.h>
#include <sys/time.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>
#include <syslog.h>
#include <unistd.h>
#include "bmp085.h"
#include "cc1101.h" 
#include "am2322.h" 

//Идентификаторы радиодатчиков
#define RADIO_1_SENSOR_ID   0x11
#define RADIO_2_SENSOR_ID   0x12

//Период опроса локально подключенных датчиков в секундах
#define ALARM_INTERVAL  600
//#define ALARM_INTERVAL    10

//Настройки SPI
#define SPI_CHANNEL         0
#define SPI_SPEED           500000

//Подключение СС1101
#define CC1101_GDO2         21
#define CC1101_GDO0         22

//Флаг остановки
char stop = 0;
 
/**
* Запись в базу
**/
void writeDataToDb(const char* sensor, double data)
{
    MYSQL *mysqlHandler;
    MYSQL *mysqlConnect;
    mysqlHandler = mysql_init(NULL);
    mysqlConnect = mysql_real_connect(mysqlHandler, "localhost", "weather", "weather", "weather", 0, NULL, 0);

    if ( mysqlConnect == NULL ){
        printf("Error connect tu mysql. %s\n", (char*)mysql_error(mysqlHandler)); 
        return;
    }

    char buffer[256];
    //Данные истории
    sprintf (buffer, "INSERT INTO history_data (parameter, update_date, param_value) VALUES ('%s',NOW(),%3.1f)", sensor, data); 
    int mysqlStatus = 0;
    mysqlStatus = mysql_query( mysqlHandler, buffer); 
    if (mysqlStatus){
        printf("Error exec query. %s\n", (char*)mysql_error(mysqlHandler)); 
        return;
    }
    
    //Текущие данные
    //Очистка строки запроса
    memset(&buffer[0], 0, sizeof(buffer));
    //Пытаемся проапдейтить
    sprintf (buffer, "update current_data set update_date=NOW(), param_value=%3.1f where parameter = '%s'", data, sensor);
    mysqlStatus = mysql_query(mysqlHandler, buffer);
    if (mysqlStatus){
        printf("Error exec query. %s\n", (char*)mysql_error(mysqlHandler)); 
        return;
    }
    //Если ничего не проапдейтелось значит надо создать
    long updatedRows = (long)mysql_affected_rows(mysqlHandler);
    if (updatedRows == 0)
    {
        //Очистка строки запроса
        memset(&buffer[0], 0, sizeof(buffer));
        sprintf (buffer, "INSERT INTO current_data (parameter, update_date, param_value) VALUES ('%s',NOW(),%3.1f)", sensor, data); 
        mysqlStatus = mysql_query(mysqlHandler, buffer);
        if (mysqlStatus){
            printf("Error exec query. %s\n", (char*)mysql_error(mysqlHandler)); 
            return;
        }
    }           
    mysql_close(mysqlHandler);  
}


/*
* Получение данных с датчика AM2322
*/
void readAM2322()
{
    Am2322 am2322;
    if (am2322.read() < 0)
    {
        printf("Error read AM2322\n");
    }
    else
    {
        printf("AM2322 Temperature=%3.1f Humidity=%3.1f\n", am2322.temperature, am2322.humidity);
        syslog(LOG_NOTICE, "AM2322 Temperature=%3.1f Humidity=%3.1f\n", am2322.temperature, am2322.humidity); 

        // Записываем в базу данных
        writeDataToDb("am2322_temperatute", am2322.temperature);
        writeDataToDb("am2322_humidity", am2322.humidity);
    }
}

void readBmp085()
{
    Bmp085 bmp085;
    int res = bmp085.read();
    if (res < 0){
        printf("Error read bmp085\n");
    }else{
        printf("BMP85 Temperatute: %3.1f, Pressure: %4.1f mm (%ld Pa)\n", bmp085.temp, bmp085.press, bmp085.pressPa); 
        syslog(LOG_NOTICE, "BMP85 Temperatute: %3.1f, Pressure: %4.1f mm (%ld Pa)\n", bmp085.temp, bmp085.press, bmp085.pressPa); 
    
        writeDataToDb("bmp085_temperatute", bmp085.temp);
        writeDataToDb("bmp085_pressure", bmp085.press);        
    }
}


void onShutdown(int ignored)
{
    stop = 1;
}

void cc1101_write_reg(unsigned char regAddress, unsigned char regValue){
    unsigned char buffer[2];
    buffer[0] = regAddress;
    buffer[1] = regValue;
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
}

unsigned char cc1101_read_reg(unsigned char regAddress){
    unsigned char buffer[2];
    buffer[0] = regAddress | 0x80;
    buffer[1] = 0;
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
    return buffer[1]; 
}


unsigned char cc1101_strobe(unsigned char strobe){
    unsigned char buffer[1];
    buffer[0] = strobe;
    int res = wiringPiSPIDataRW(SPI_CHANNEL, buffer, 1);
    if (res == -1){
        printf("Error send strob\n");
    }
    return buffer[0];
}

unsigned char cc1101_status(unsigned char strobe){
    unsigned char buffer[2];
    buffer[0] = strobe | 0xC0;
    buffer[1] = 0;
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, 2);
    return buffer[1];
}

void cc1101_white_fifo(unsigned char * data, unsigned char lenght){
    unsigned char buffer[lenght + 1];
    buffer[0] = FIFO_RX_BURST;
    unsigned char i;
    for(i=0; i<lenght; i++){
        buffer[i+1] = data[i];
    }
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, lenght + 1);
}

void cc1101_read_fifo(unsigned char * data, unsigned char lenght){
    unsigned char buffer[lenght + 1];
    buffer[0] = FIFO_RX_BURST;
    wiringPiSPIDataRW(SPI_CHANNEL, buffer, lenght + 1);
    unsigned char i;
    for(i=0; i<lenght; i++){
        data[i] = buffer[i+1];
    }
}

/***************************************************************
 *  SmartRF Studio(tm) Export
***************************************************************/
void cc1101_init(){
    cc1101_write_reg(IOCFG0, 0x07);
    cc1101_write_reg(FIFOTHR, 0x47);
    cc1101_write_reg(SYNC1, 0x7A);
    cc1101_write_reg(SYNC0, 0x0E);
    cc1101_write_reg(PKTLEN, 0x14);
    cc1101_write_reg(PKTCTRL1, 0x0D);
    cc1101_write_reg(PKTCTRL0, 0x45);
    cc1101_write_reg(ADDR, 0x31);
    cc1101_write_reg(FSCTRL1, 0x06);
    cc1101_write_reg(FREQ2, 0x21);
    cc1101_write_reg(FREQ1, 0x62);
    cc1101_write_reg(FREQ0, 0x76);
    cc1101_write_reg(MDMCFG4, 0xCA);
    cc1101_write_reg(MDMCFG3, 0xF8);
    cc1101_write_reg(MDMCFG2, 0x1E);
    cc1101_write_reg(MDMCFG1, 0x42);
    cc1101_write_reg(DEVIATN, 0x40);
    cc1101_write_reg(MCSM0, 0x18);
    cc1101_write_reg(MCSM1, 0x3C);
    cc1101_write_reg(FOCCFG, 0x16);
    cc1101_write_reg(AGCCTRL2, 0x43);
    cc1101_write_reg(AGCCTRL1, 0x49);
    cc1101_write_reg(WORCTRL, 0xFB);
    cc1101_write_reg(FSCAL3, 0xE9);
    cc1101_write_reg(FSCAL2, 0x2A);
    cc1101_write_reg(FSCAL1, 0x00);
    cc1101_write_reg(FSCAL0, 0x1F);
    cc1101_write_reg(TEST2, 0x81);
    cc1101_write_reg(TEST1, 0x35);
    cc1101_write_reg(TEST0, 0x09);
}


void cc1101_restart(){
    printf("cc1101_restart\n");
    
    unsigned char res;
    
    res = cc1101_strobe(SRES);
    printf("SRES=%d\n", res);
    delay(1);
    
    res = cc1101_strobe(SFRX);
    printf("SFRX=%d\n", res);   
    delay(1);

    res = cc1101_status(VERSION);
    printf("VERSION=%d\n", res);
    delay(1);

    cc1101_init();
    delay(1);
    
    res = cc1101_strobe(SRX);
    printf("SRX=%d\n", res);
    delay(1);

    res = cc1101_strobe(SNOP);
    printf("SNOP=%d\n", res);
}

void cc1101_restart_if_need(){
    unsigned char res;
    res = cc1101_strobe(SNOP);
    printf("SNOP=%d\n", res);

    if ((res & STATE_MASK) != RX){
        printf("Restart CC1101\n");
        cc1101_restart();
    }
}


void onTimer(int ignored)
{
    //Опрашиваем локальные датчики
    readBmp085();
    readAM2322();

    // Вывод статуса приемника
    unsigned short res = cc1101_strobe(SNOP);
    printf("CC1101 Staus=%d\n", res);
    
    cc1101_restart_if_need();
    
    //Запускаем таймер
    alarm(ALARM_INTERVAL);
}

void onGDO0Change(){
    unsigned char count;
    count = cc1101_status(RXBYTES);
    delay(1);
    printf("Recive %i byte\n", count);
    if (count > 0){
        unsigned char data[count];
        cc1101_read_fifo(data, count);
        delay(1);
        printf("Data: ");
        unsigned char i;
        for (i=0; i<count; i++){
            printf("%i ", data[i]);
        }
        printf("\n");

        if (count == 17 && (data[2]==RADIO_1_SENSOR_ID || data[2]==RADIO_2_SENSOR_ID)){
            float temperature, humidity, vcc;
            memcpy(&temperature, &data[3], sizeof(temperature));
            memcpy(&humidity, &data[7], sizeof(humidity));
            memcpy(&vcc, &data[11], sizeof(vcc));
            printf("Radio sensor %i Temperature=%3.1f Humidity=%3.1f Vcc=%3.1f\n", data[2], temperature, humidity, vcc);
            syslog(LOG_NOTICE, "Radio sensor %i Temperature=%3.1f Humidity=%3.1f, Vcc=%3.1f\n", data[2], temperature, humidity, vcc);

            //Запись в базу данных
            char sensor_description[64];
            sprintf (sensor_description, "radio_%d_temperature", data[2]);
            writeDataToDb(sensor_description, temperature);

            memset(&sensor_description[0], 0, sizeof(sensor_description));
            sprintf (sensor_description, "radio_%d_humidity", data[2]);
            writeDataToDb(sensor_description, humidity);

			memset(&sensor_description[0], 0, sizeof(sensor_description));
            sprintf (sensor_description, "radio_%d_vcc", data[2]);
            writeDataToDb(sensor_description, vcc);
        }else if(count == 9 && data[3]=='e' && data[4]=='r' && data[5]=='r'){
            printf("ERROR in radio sensor %i: %i\n", data[2], data[6]);
            syslog(LOG_NOTICE, "ERROR in radio sensor %i: %i\n", data[2], data[6]);
        }else{
            printf("ERROR in radio sensor %i\n", data[2]);
            syslog(LOG_NOTICE, "ERROR in radio sensor %i\n", data[2]);
            cc1101_restart_if_need();
        }
    }else{
        cc1101_restart_if_need();
    }
}



int main (void)
{
    //Подписываемся на CTRL+C, kill итд
    signal((int)SIGTERM, onShutdown);
    signal(SIGKILL, onShutdown);
    signal(SIGINT, onShutdown);
    //Подписываемся на таймер
    signal(SIGALRM, onTimer);       
    
    openlog("weather-server", LOG_CONS, LOG_USER);
    syslog(LOG_NOTICE, "Start weather server. V 1.0.4");
    printf("Weather server. V 1.0.4\n");
    
    wiringPiSetup();
    
    int spiHandler = wiringPiSPISetup (SPI_CHANNEL, SPI_SPEED);
    if (spiHandler == -1){
        printf("Error init spi interface\n");
    }
    
    cc1101_restart();
    
    // Подписываемся на приход данных на тюнер
    wiringPiISR(CC1101_GDO0, INT_EDGE_RISING, onGDO0Change);    

    readAM2322();

    //Запускаем таймер
    alarm(ALARM_INTERVAL);
    while(1)
    {
        delay(1000);
        
        //Проверяем флаг остановки приложения
        if (stop)
        {
            printf("Server is shut down\n");
            break;
        }
    }
    close(spiHandler);
    //Закрытие файла лога
    closelog();
    return 0;
}
