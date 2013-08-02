/*
 * spi-ram.h
 *
 *  Created on: 2013-07-28
 *      Author: Jacques Deschênes
 *      Description:  interface avec RAM SPI Microchip 23Kyyy  ou 23Ayyy
 */

#ifndef SPI_RAM_H_
#define SPI_RAM_H_
#include <msp430.h>

//  E/S
#define SPIDIR  P1DIR
#define SPISEL  P1SEL
#define SPISEL2 P1SEL2
#define CS_DIR  P2DIR
#define CS_OUT  P2OUT

#define SRAM_OUT BIT7  // P1.7  UCB0_SIMO  serial RAM SI
#define SRAM_IN  BIT6  // P1.6  UCB0_SOMI  serial RAM SO
#define SRAM_CLK BIT5  // P1.5  UCB0_CLK   serial RAM CLK
#define SRAM_CS  BIT2  // P2.2  serial RAM chip select
// registres
#define SRAM_TXBUF UCB0TXBUF
#define SRAM_RXBUF UCB0RXBUF
#define SRAM_IFG 	IFG2
#define SRAM_CTL0 UCB0CTL0
#define SRAM_CTL1 UCB0CTL1
#define SRAM_BR0  UCB0BR0
#define SRAM_BR1  UCB0BR1
#define SRAM_SPI_STAT UCB0STAT
#define SRAM_RXIFG  UCB0RXIFG
#define SRAM_TXIFG	UCB0TXIFG
// clock source
#define SRAM_NOCLK  0
#define SRAM_ACLK  (1<<6)
#define SRAM_SMCLK (1<<7)

// opérations
#define SRAM_READ	3
#define SRAM_WRITE	2
#define SRAM_RDSR	5
#define SRAM_WRSR	1


#define SRAM_BYTE_MODE		0
#define SRAM_PAGE_MODE		(1<<7)
#define SRAM_SEQ_MODE 		(1<<6)


#define _enable_sram()  CS_OUT &= ~SRAM_CS;
#define _disable_sram() CS_OUT |= SRAM_CS
#define _wait_txifg() while (!(IFG2 & SRAM_TXIFG))
#define _wait_rxifg() while (!(IFG2 & SRAM_RXIFG))

void sram_init(unsigned int mode,unsigned char clock, unsigned int divisor);
void set_sram_mode(unsigned int mode);
void read_sram_bytes(unsigned int address, unsigned char *buffer, unsigned int count);
void write_sram_bytes(unsigned int address, const unsigned char *buffer, unsigned int count);
unsigned char read_sram_status();
void write_sram_status(unsigned char status);


#endif /* SPI_RAM_H_ */
