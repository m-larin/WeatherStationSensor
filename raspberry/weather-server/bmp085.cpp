
#include <wiringPiI2C.h>
#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include "bmp085.h"


void Bmp085::calibrate(int dev) 
{
    ac1 = readShortWord(dev, COLEBRATION_REG);
    ac2 = readShortWord(dev, COLEBRATION_REG + 2);
    ac3 = readShortWord(dev, COLEBRATION_REG + 4);
    ac4 = readUShortWord(dev, COLEBRATION_REG + 6);
    ac5 = readUShortWord(dev, COLEBRATION_REG + 8);
    ac6 = readUShortWord(dev, COLEBRATION_REG + 10);
    b1 = readShortWord(dev, COLEBRATION_REG + 12);
    b2 = readShortWord(dev, COLEBRATION_REG + 14);
    mb = readShortWord(dev, COLEBRATION_REG + 16);
    mc = readShortWord(dev, COLEBRATION_REG + 18);
    md = readShortWord(dev, COLEBRATION_REG + 20);
    
    //Test data
    /*ac1 = 408;
    ac2 = -72;
    ac3 = -14383;
    ac4 = 32741;
    ac5 = 32757;
    ac6 = 23153;
    b1 = 6190;
    b2 = 4;
    mb = -32768;
    mc = -8711;
    md = 2868;*/
    //printf("Calibration coefficients: %d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", ac1, ac2, ac3, ac4, ac5, ac6, b1, b2, mb, mc, md);
}

long Bmp085::get_temp(long ut) 
{
    long  tval;
    long  x1, x2, b5;

    x1 = (ut - ac6) * ac5 >> 15;
    x2 = ((long ) mc << 11) / (x1 + md);
    b5 = x1 + x2;
    tval = (b5 + 8) >> 4;   
    return tval;
}

long Bmp085::get_press(long up, long ut) 
{
    long  pval;
    long  x1, x2, x3, b3, b5, b6, p;
    unsigned long  b4, b7;
    
    x1 = (ut - ac6) * ac5 >> 15;
    x2 = ((long ) mc << 11) / (x1 + md);
    b5 = x1 + x2;
    
    b6 = b5 - 4000;
    x1 = (b2 * (b6 * b6 >> 12)) >> 11; 
    //printf("x1=%d\n", x1);
    x2 = ac2 * b6 >> 11;
    //printf("x2=%d\n", x2);
    x3 = x1 + x2;
    b3 = ((((long ) ac1 * 4 + x3)<<OSS) + 2) >> 2;
    //printf("b3=%d\n", b3);
    x1 = ac3 * b6 >> 13;
    //printf("x1=%d\n", x1);
    x2 = (b1 * (b6 * b6 >> 12)) >> 16;
    //printf("x2=%d\n", x2);
    x3 = ((x1 + x2) + 2) >> 2;
    //printf("x3=%d\n", x3);
    b4 = (ac4 * (unsigned long ) (x3 + 32768)) >> 15;
    //printf("b4=%d\n", b4);
    b7 = ((unsigned long ) up - b3) * (50000 >> OSS);
    //printf("b7=%d\n", b7);
    p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
    //printf("p=%d\n", p);

    x1 = (p >> 8) * (p >> 8);
    //printf("x1=%d\n", x1);
    x1 = (x1 * 3038) >> 16;
    //printf("x1=%d\n", x1);
    x2 = (-7357 * p) >> 16;
    //printf("x2=%d\n", x2);
    pval = p + ((x1 + x2 + 3791) >> 4);
    return pval;
}


int Bmp085::read(void)
{
    int dev = wiringPiI2CSetup (0x77) ;
    if (dev < 0)
    {
        printf("Error. Devise not init\n");
        return dev;
    }
    calibrate(dev);

    wiringPiI2CWriteReg8(dev, 0xF4, 0x2E);
    delay(5);
    long ut = readShortWord(dev, 0xF6);
    //ut = 27898;
    //printf("ut=%d\n", ut);

    wiringPiI2CWriteReg8(dev, 0xF4, 0x34 + (OSS<<6));
    delay(30);
    unsigned short msb = wiringPiI2CReadReg8(dev, 0xF6);
    unsigned short lsb = wiringPiI2CReadReg8(dev, 0xF7);
    unsigned short xlsb = wiringPiI2CReadReg8(dev, 0xF8);
    long up = (((long)msb<<16)+((long)lsb<<8)+xlsb)>>(8-OSS);
    //up = 23843;
    //printf("up=%d\t%d\t%d\t%d\n", up, msb, lsb, xlsb);

    long temp10 = get_temp(ut);    
    temp = ((double)temp10) / (double)10;
    
    pressPa = get_press(up, ut);
    press = ((double)pressPa) * (double)760 / (double)101325;

    close(dev);
    
    return 0;
}

short Bmp085::readShortWord(int dev, int address)
{
    unsigned char msb, lsb;
    msb = wiringPiI2CReadReg8(dev, address);
    lsb = wiringPiI2CReadReg8(dev, address + 1);
    short result = ((short)msb<<8) | lsb;
    return result;
}

unsigned short Bmp085::readUShortWord(int dev, int address)
{
    unsigned char msb, lsb;
    msb = wiringPiI2CReadReg8(dev, address);
    lsb = wiringPiI2CReadReg8(dev, address + 1);
    unsigned short result = ((short)msb<<8) | lsb;
    return result;
}

