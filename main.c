/*
 *    DATE: 2013-07-28
 *    AUTEUR: Jacques Deschênes
 *    DESCRIPTION: Ajout d'une mémoire RAM SPI externe pour
 *    		       augmenter la résolution de l'affichage à 200x200
 *
 */
#include <msp430.h>
#include <string.h>
#include "spi-ram.h"
#include "main.h"


#define _blank_level()  P2OUT &= ~BLK_OUT;\
						P2DIR |= BLK_OUT

#define _black_level()  P2DIR &= ~BLK_OUT;

#define SYNC_OUT  BIT1 // P2.1 TA1CCR0 output, synchro NTSC

#define  DISP_BLK 1 // affichage bloqué

volatile unsigned int ln_cnt; // compte les ligne balyage NTSC
volatile unsigned char blanked; // indicateur d'affichage bloquée pendant phase vsync.
volatile unsigned char video_op; // opération sur mémoire vidéo
volatile unsigned int sram_addr; // addresse début opération lecture/écriture
volatile unsigned int byte_count; // nombre d'octets à lire ou écrire
volatile unsigned  long ticks; // compteur système incrémentée à toute les 62,5µSec.
volatile unsigned int video_addr; // addresse de début de la mémoire vidéo dans la SRAM
volatile unsigned int mcu_addr; // pointeur vers buffer vidéo en RAM

// police de caractères 5x7  lettres majuscules et chiffre
const unsigned char font[][7]={
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // espace
	{0x04,0x04,0x04,0x04,0x04,0x00,0x04}, // !
	{0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00}, // "
	{0x0a,0x0a,0x1f,0x0a,0x1f,0x0a,0x0a}, // #
	{0x04,0x0f,0x14,0x0e,0x05,0x1e,0x04}, // $
	{0x18,0x19,0x02,0x04,0x08,0x13,0x03}, // %
	{0x0c,0x12,0x14,0x08,0x15,0x12,0x0d}, // &
	{0x0c,0x04,0x08,0x00,0x00,0x00,0x00}, // '
	{0x02,0x04,0x08,0x08,0x08,0x04,0x02}, // (
	{0x08,0x04,0x02,0x02,0x02,0x04,0x08}, // )
	{0x00,0x04,0x15,0x0e,0x15,0x04,0x00}, // *
	{0x00,0x04,0x04,0x1f,0x04,0x04,0x00}, // +
	{0x00,0x00,0x00,0x00,0x0c,0x04,0x08}, // ,
	{0x00,0x00,0x00,0x1e,0x00,0x00,0x00}, // -
	{0x00,0x00,0x00,0x00,0x00,0x0c,0x0c}, // .
	{0x00,0x01,0x02,0x04,0x08,0x10,0x00}, // /
	{0x0e,0x11,0x13,0x15,0x19,0x11,0x0e}, // 0
	{0x04,0x0c,0x04,0x04,0x04,0x04,0x1f}, // 1
	{0x0e,0x11,0x02,0x04,0x08,0x10,0x1f}, // 2
	{0x1e,0x01,0x01,0x1e,0x01,0x01,0x1e}, // 3
	{0x02,0x06,0x0a,0x12,0x1f,0x02,0x02}, // 4
	{0x1f,0x10,0x10,0x1e,0x01,0x01,0x1e}, // 5
	{0x06,0x08,0x10,0x1e,0x11,0x11,0x0e}, // 6
	{0x1f,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
	{0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e}, // 8
	{0x0e,0x11,0x11,0x0e,0x01,0x01,0x0e}, // 9
	{0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x00}, // :
	{0x00,0x0c,0x0c,0x00,0x0c,0x04,0x08}, // ;
	{0x02,0x04,0x08,0x10,0x08,0x04,0x02}, // <
	{0x00,0x00,0x1f,0x00,0x1f,0x00,0x00}, // =
	{0x08,0x04,0x02,0x01,0x02,0x04,0x08}, // >
	{0x0e,0x11,0x01,0x02,0x04,0x00,0x04}, // ?
	{0x0e,0x11,0x01,0x0d,0x15,0x15,0x0e}, // @
	{0x0e,0x11,0x11,0x1f,0x11,0x11,0x11}, // A
	{0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e}, // B
	{0x0f,0x10,0x10,0x10,0x10,0x10,0x0f}, // C
	{0x1e,0x11,0x11,0x11,0x11,0x11,0x1e}, // D
	{0x1f,0x10,0x10,0x1f,0x10,0x10,0x1f}, // E
	{0x1f,0x10,0x10,0x1f,0x10,0x10,0x10}, // F
	{0x0f,0x10,0x10,0x16,0x11,0x11,0x0e}, // G
	{0x11,0x11,0x11,0x1f,0x11,0x11,0x11}, // H
	{0x0e,0x04,0x04,0x04,0x04,0x04,0x0e}, // I
	{0x0f,0x01,0x01,0x01,0x01,0x12,0x0c}, // J
	{0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
	{0x10,0x10,0x10,0x10,0x10,0x10,0x1f}, // L
	{0x11,0x1b,0x15,0x11,0x11,0x11,0x11}, // M
	{0x11,0x11,0x19,0x15,0x13,0x11,0x11}, // N
	{0x0e,0x11,0x11,0x11,0x11,0x11,0x0e}, // O
	{0x1e,0x11,0x11,0x1e,0x10,0x10,0x10}, // P
	{0x0e,0x11,0x11,0x11,0x15,0x13,0x0f}, // Q
	{0x1e,0x11,0x11,0x1e,0x14,0x12,0x11}, // R
	{0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e}, // S
	{0x1f,0x04,0x04,0x04,0x04,0x04,0x04}, // T
	{0x11,0x11,0x11,0x11,0x11,0x11,0x0e}, // U
	{0x11,0x11,0x11,0x11,0x11,0x0a,0x04}, // V
	{0x11,0x11,0x11,0x15,0x15,0x1b,0x11}, // W
	{0x11,0x11,0x0a,0x04,0x0a,0x11,0x11}, // X
	{0x11,0x11,0x11,0x0a,0x04,0x04,0x04}, // Y
	{0x1f,0x02,0x04,0x08,0x10,0x10,0x1f}, // Z
	{0x0c,0x08,0x08,0x08,0x08,0x08,0x0c}, // [
	{0x00,0x10,0x08,0x04,0x02,0x01,0x00}, // '\'
	{0x03,0x01,0x01,0x01,0x01,0x01,0x03}, // ]
	{0x04,0x0a,0x11,0x00,0x00,0x00,0x00}, // ^
	{0x00,0x00,0x00,0x00,0x00,0x00,0x1f}, // _
	{0x08,0x04,0x02,0x00,0x00,0x00,0x00}, // `
};


unsigned char disp_buffer[7*BYTES_PER_LINE]; // tampon pour le transfert de données entre mémoire RAM et SPI RAM

void delay_ms(unsigned int ms){
	while (ms--){
		while (ticks&15);
		while (!(ticks&15));
	}
} // delay_ms()

void configure_clock(){ // MCLK et SMCLK à 16Mhz
	 BCSCTL1 = CALBC1_16MHZ;
	 DCOCTL = CALDCO_16MHZ;
}//configure_clock()



void clear_screen(){
	sram_addr=0;
	byte_count=SCREEN_HEIGHT;
	video_op=VID_CLR;
	while (video_op);

}//clear_screen()


void dot(int x, int y){
	if ((x>>3)>=BYTES_PER_LINE) return;
	disp_buffer[0] = (1 << (7-(x & 7)));
	sram_addr=(y<<5)+(x>>3); // 48 octets par lignes, sram_addr=y*BYTES_PER_LINE + x/8
	mcu_addr=(unsigned int)(&disp_buffer[0]);
	video_op=VID_SET;
	while (video_op);
} // dot()

void erase_dot(int x, int y){
	if ((x>>3)>=BYTES_PER_LINE) return;
	disp_buffer[0] = (1 << (7-(x & 7)));
	sram_addr=(y<<5)+(x>>3);
	mcu_addr=(unsigned int)(&disp_buffer[0]);
	_disable_interrupts();
	video_op=VID_RST;
	_enable_interrupts();
	while (video_op);
}//erase_dot()


void swap(int* n1, int* n2){
	int t;
	t=*n1;
	*n1=*n2;
	*n2=t;
}

// REF: http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
void draw_line(int x0, int y0, int x1, int y1){
 int dx,dy, error,ystep,x,y;
 unsigned char steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
      swap(&x0, &y0);
      swap(&x1, &y1);
  }
  if (x0 > x1){
      swap(&x0, &x1);
      swap(&y0, &y1);
  }
   dx = (x1 - x0);
  dy = abs(y1 - y0);
  error = dx>>1;
  y = y0;
  if (y0 < y1)  ystep = 1; else ystep = -1;
  for (x=x0;x<=x1;x++){
      if (steep) dot(y,x); else dot(x,y);
      error = error - dy;
      if (error < 0){
          y = y + ystep;
          error = error + dx;
      }
  }
} // line()


void rectangle(int x0, int y0, int x1, int y1){
	draw_line(x0,y0,x1,y0);
	draw_line(x1,y0,x1,y1);
	draw_line(x0,y1,x1,y1);
	draw_line(x0,y0,x0,y1);
} // rectangle()


/*
 *  écris le caractère dans disp_buffer.
 *  utilise disp_buffer[] comme un tableau 2D disp_buffer[BYTES_PER_LINE][7]
 */
void draw_char(int x,  unsigned char c){
unsigned char shift,row;
unsigned int idx, byte;

	if (c>='a') c -=32;
    if (c<' ' || c>'z') return;
    c -= ' ';
	shift=11-(x&7);
	x >>= 3;
	for (row=0;row<7;row++){
    	idx = (row<<5)+x; // BYTES_PER_LINE=32
    	byte=font[c][row];
		if (shift>7){
			disp_buffer[idx] |=byte<<(shift-8);
		}else{
			disp_buffer[idx] |=byte>>(8-shift);
			disp_buffer[idx+1] |=byte<<(shift);
		}
	}// for (row...
} // draw_char()


const char msg[]="MSP430G2553 B/W NTSC video demo. 256x230.";


void print(int x, int y, const char* str){
int idx=0;
	// lire mémoire spi qui va contenir la chaîne de caractère
	byte_count=BYTES_PER_LINE;
	mcu_addr=(unsigned int)&disp_buffer[0];
	sram_addr=y<<5;
	for (idx=0;idx<7;idx++){
		video_op=VID_READ;
		while (video_op);
		sram_addr += BYTES_PER_LINE;
		mcu_addr  += BYTES_PER_LINE;
	}
	// écrire la chaîne dans disp_buffer
	idx=0;
	while  (x<((BYTES_PER_LINE<<3)-6) && (str[idx]!=0)){
		draw_char(x,str[idx]);
		idx++;
		x+=6;
	}
	// retransférer le buffer dans la SPI RAM
	sram_addr=(y<<5);
	mcu_addr=(unsigned int)&disp_buffer[0];
	for (idx=0;idx<7;idx++){
		video_op=VID_WRITE;
		while(video_op);
		sram_addr += BYTES_PER_LINE;
		mcu_addr += BYTES_PER_LINE;
	}
}// print()



#include "loup.h"
#define MARGIN ((BYTES_PER_LINE-IMG_WIDTH)>>1)
void copy_img_2_sram(){//copie l'image qui est dans la flash vers SPI RAM
	unsigned int i,l, addr;
	for (i=0;i<MARGIN;i++){ // efface les marges
		disp_buffer[i]=0;
		disp_buffer[i+MARGIN+IMG_WIDTH]=0;
	}
	addr=0;
//	disp_buffer[0]=0x80; 				 // ligne verticale à gauche
//	disp_buffer[BYTES_PER_LINE-1]=0x01; // et à droite
	for (l=0;l<IMG_HEIGHT;l++){
		for (i=0;i<IMG_WIDTH;i++){
			disp_buffer[i+MARGIN]=loup[l][i]; // image centrée horizontalement
		}
		write_sram_bytes(addr,&disp_buffer[0],BYTES_PER_LINE);
		addr += BYTES_PER_LINE;
	}
	for (i=0;i<BYTES_PER_LINE;i++){
		disp_buffer[i]=255;
	}
}// copy_2_sram()


/*
 * main.c
 */
void main(void) {

	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    configure_clock();
    P2DIR = SYNC_OUT;  // P2.1 en sortie
    P2SEL = SYNC_OUT; //minuterie sur P2.1
    // configuration SPI RAM
    sram_init(SRAM_SEQ_MODE,SRAM_SMCLK, SMCLK_DIV);
    copy_img_2_sram();
    //configuration minuterie 1 pour syncho NTSC
    TA1CCR0=H_LINE;
    TA1CCR1=H_LINE-H_SYNC; // commence avec une synchro verticale
    TA1CCR2=H_DISPLAY_DELAY;
    TA1CCTL1=OUTMOD_3; // mode toggle/set
    TA1CCTL0 = CCIE;
    TA1CCTL2 |= CCIE;
    TA1CTL = TASSEL_2+MC_1; // SMCLK, UP mode
    _enable_interrupts();
    video_op=VID_NONE;
    video_addr=0;
	ln_cnt=1;
	blanked=1;
	_blank_level();
	sram_addr=221<<5;
	byte_count=9;
	video_op=VID_CLR;
	while (video_op);
	print(0,221,&msg[0]);
	while (1){
	}//while(1)
}//main()


#pragma vector=ADC10_VECTOR
#pragma vector=COMPARATORA_VECTOR
#pragma vector=NMI_VECTOR
#pragma vector=PORT1_VECTOR
#pragma vector=PORT2_VECTOR
#pragma vector=TIMER0_A0_VECTOR
#pragma vector=TIMER0_A1_VECTOR
#pragma vector=USCIAB0RX_VECTOR
#pragma vector=USCIAB0TX_VECTOR
#pragma vector=WDT_VECTOR
__interrupt void unsused(void){
	return;
}
