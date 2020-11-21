#ifndef CC1101_H
#define CC1101_H

// Регистры
#define IOCFG2 0x0000
#define IOCFG1 0x0001
#define IOCFG0 0x0002
#define FIFOTHR 0x0003
#define SYNC1 0x0004
#define SYNC0 0x0005
#define PKTLEN 0x0006
#define PKTCTRL1 0x0007
#define PKTCTRL0 0x0008
#define ADDR 0x0009
#define CHANNR 0x000A
#define FSCTRL1 0x000B
#define FSCTRL0 0x000C
#define FREQ2 0x000D
#define FREQ1 0x000E
#define FREQ0 0x000F
#define MDMCFG4 0x0010
#define MDMCFG3 0x0011
#define MDMCFG2 0x0012
#define MDMCFG1 0x0013
#define MDMCFG0 0x0014
#define DEVIATN 0x0015
#define MCSM2 0x0016
#define MCSM1 0x0017
#define MCSM0 0x0018
#define FOCCFG 0x0019
#define BSCFG 0x001A
#define AGCCTRL2 0x001B
#define AGCCTRL1 0x001C
#define AGCCTRL0 0x001D
#define WOREVT1 0x001E
#define WOREVT0 0x001F
#define WORCTRL 0x0020
#define FREND1 0x0021
#define FREND0 0x0022
#define FSCAL3 0x0023
#define FSCAL2 0x0024
#define FSCAL1 0x0025
#define FSCAL0 0x0026
#define RCCTRL1 0x0027
#define RCCTRL0 0x0028
#define FSTEST 0x0029
#define PTEST 0x002A
#define AGCTEST 0x002B
#define TEST2 0x002C
#define TEST1 0x002D
#define TEST0 0x002E
#define PARTNUM 0x0030
#define VERSION 0x0031
#define FREQEST 0x0032
#define LQI 0x0033
#define RSSI 0x0034
#define MARCSTATE 0x0035
#define WORTIME1 0x0036
#define WORTIME0 0x0037
#define PKTSTATUS 0x0038
#define VCO_VC_DAC 0x0039
#define TXBYTES 0x003A
#define RXBYTES 0x003B
#define RCCTRL1_STATUS 0x003C
#define RCCTRL0_STATUS 0x003D

// ================ Команды ===========
#define SRES 0x30
#define SFSTXON 0x31
#define SXOFF 0x32
#define SCAL 0x33
#define SRX 0x34
#define STX 0x35
#define SIDLE 0x36
#define SWOR 0x38
#define SPWD 0x39
#define SFRX 0x3A
#define SFTX 0x3B
#define SWORRST 0x3C
#define SNOP 0x3D

//FIFO
#define FIFO_TX_SINGLE 0x3F
#define FIFO_TX_BURST 0x7F
#define FIFO_RX_SINGLE 0xBF
#define FIFO_RX_BURST 0xFF

//PATABLE
#define PATABLE 0x3E
#define PATABLE_10 0xC2

// STATE
#define STATE_MASK 0x70 
#define IDLE 0x00   
#define RX 0x10

#endif //CC1101_H
