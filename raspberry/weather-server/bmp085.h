#ifndef BMP085_H
#define BMP085_H

//Стартовый адрес калибровочных данных для BMP085
#define COLEBRATION_REG     0xAA
//Точность измерения данных для BMP085 3-максимальная точность
#define OSS                 3

class Bmp085
{
private:
    //Калибровочные переменные для bmp085
    short ac1, ac2, ac3, b1, b2, mb, mc, md;
    unsigned short ac4, ac5, ac6;

    void calibrate(int dev);
    long get_temp(long ut);
    long get_press(long up, long ut);
    short readShortWord(int dev, int address);
    unsigned short readUShortWord(int dev, int address);
public:
    int read(void);
    double temp;
    double press;
    long pressPa;
};
#endif //BMP085
