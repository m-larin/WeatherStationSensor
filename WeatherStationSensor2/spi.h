#ifndef SPI_H_
#define SPI_H_

// ���������� ������� ����� SPI
#define SPI_PORT	PORTA	// SPI ����
#define SPI_DDR		DDRA	// SPI DDR
#define SPI_PIN		PINA	// SPI PIN
#define SPI_MOSI	PORTA6	// ����� - MOSI
#define SPI_MISO	PORTA5	// ����� - MISO
#define SPI_SCK		PORTA4	// ����  - SCK
#define SPI_SS		PORTA1	// ����� - Chip Select


//�������������
void spi_init();

// ����� ������������������
void spi_start(void);

// ���� ������������������
void spi_stop(void);

// ������� ����
unsigned char spi_sendbyte(unsigned char data);

// ��������� ����
unsigned char spi_readbyte(void);

#endif /* SPI_H_ */