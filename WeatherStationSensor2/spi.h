#ifndef SPI_H_
#define SPI_H_

// назначение выводов порта SPI
#define SPI_PORT	PORTA	// SPI Порт
#define SPI_DDR		DDRA	// SPI DDR
#define SPI_PIN		PINA	// SPI PIN
#define SPI_MOSI	PORTA6	// выход - MOSI
#define SPI_MISO	PORTA5	// выход - MISO
#define SPI_SCK		PORTA4	// вход  - SCK
#define SPI_SS		PORTA1	// выход - Chip Select


//Инициализация
void spi_init();

// СТАРТ последовательность
void spi_start(void);

// СТОП последовательность
void spi_stop(void);

// ПОСЛАТЬ байт
unsigned char spi_sendbyte(unsigned char data);

// ПРОЧИТАТЬ байт
unsigned char spi_readbyte(void);

#endif /* SPI_H_ */