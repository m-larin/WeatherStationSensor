#ifndef AM2322_H
#define AM2322_H

#define AM2322_ADDRESS  0x5C

class Am2322
{
private:
    unsigned short crc16(unsigned char *ptr, unsigned char len);
public:
    double temperature;
    double humidity;

    int read(void);
};

#endif //AM2322_H
