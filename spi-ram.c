/*
 * spi-ram.c
 *
 *  Created on: 2013-07-28
 *      Author: Jacques Deschênes
 *      Description: interface avec mémoire RAM SPI  Microchip 23Kyyy  ou 23Ayyy
 *
 */

#include "spi-ram.h"

void sram_init(unsigned int mode,unsigned char clock, unsigned int divisor){
	SPIDIR |= SRAM_OUT+SRAM_CLK;
	SPISEL |=SRAM_IN+SRAM_OUT+SRAM_CLK;
	SPISEL2|=SRAM_IN+SRAM_OUT+SRAM_CLK;
	CS_DIR |= SRAM_CS;
	_disable_sram();
	SRAM_CTL1 |= UCSWRST;
	SRAM_CTL0 |= UCMSB+UCMST+UCSYNC+UCCKPL;
	SRAM_BR0 = divisor;
	SRAM_BR1 = divisor>>8;
	SRAM_CTL1 |= clock;
	SRAM_CTL1 &= ~UCSWRST;
	set_sram_mode(mode);
}//sram_init()

void set_sram_mode(unsigned int mode){
	_enable_sram();
	SRAM_TXBUF = SRAM_WRSR;
	_wait_txifg();
	SRAM_TXBUF = mode;
	while (SRAM_SPI_STAT & UCBUSY);
	_disable_sram();
}// set_sram_mode()


void read_sram_bytes(unsigned int address, unsigned char *buffer, unsigned int count){
	unsigned int i;
	_enable_sram();
	SRAM_TXBUF = SRAM_READ;
	_wait_txifg();
	SRAM_TXBUF=address>>8;
	_wait_txifg();
	SRAM_TXBUF=address &0xff;
	_wait_txifg();
	*buffer=SRAM_RXBUF;
	for (i=0;i<count;i++){
		SRAM_TXBUF=0;
		_wait_rxifg();
		*buffer++=SRAM_RXBUF;
	}
	while (SRAM_SPI_STAT & UCBUSY);
	_disable_sram();
}// read_sram_bytes()

void write_sram_bytes(unsigned int address, const unsigned char *buffer, unsigned int count){
	unsigned int i;
	_enable_sram();
	SRAM_TXBUF=SRAM_WRITE;
	_wait_txifg();
	SRAM_TXBUF=address>>8;
	_wait_txifg();
	SRAM_TXBUF=address;
	_wait_txifg();
	for (i=0;i<count;i++){
		SRAM_TXBUF = *buffer++;
		_wait_txifg();
	}
	while (SRAM_SPI_STAT & UCBUSY);
	_disable_sram();
}//write_sram_bytes()

unsigned char read_sram_status(){
	_enable_sram();
	SRAM_TXBUF=SRAM_RDSR;
	_wait_txifg();
	SRAM_TXBUF=0;
	_wait_rxifg();
	while (SRAM_SPI_STAT & UCBUSY);
	_disable_sram();
	return SRAM_RXBUF;
}// read_sram_status()

void write_sram_status(unsigned char status){
	_enable_sram();
	SRAM_TXBUF=SRAM_WRSR;
	_wait_txifg();
	SRAM_TXBUF=status;
	while (SRAM_SPI_STAT & UCBUSY);
	_disable_sram();
}//write_sram_status()
