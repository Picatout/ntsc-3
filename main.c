/*
 *    DATE: 2013-08-07
 *    AUTEUR: Jacques Deschênes
 *    DESCRIPTION: test copie page vidéos
 *    		       utilisant une mémoire RAM externe 23K256
 *				   Avec une résolution de 256x240 la mémoire
 *				   peut contenir 4 pages vidéo.
 *				   Ce démo utilise les pages 0 et 1
 *				   en copiant l'image du loup d'une page à l'autre.
 */
#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include "spi-ram.h"
#include "main.h"

#define _blank_level()  P2OUT &= ~BLK_OUT;\
						P2DIR |= BLK_OUT

#define _black_level()  P2DIR &= ~BLK_OUT;

#define SYNC_OUT  BIT1 // P2.1 TA1CCR0 output, synchro NTSC

volatile unsigned int ln_cnt; // compte les ligne balyage NTSC
volatile unsigned char blanked; // indicateur d'affichage bloquée pendant phase vsync.
volatile unsigned char video_op; // opération sur mémoire vidéo
volatile unsigned int sram_addr; // addresse début opération lecture/écriture
volatile unsigned int byte_count; // nombre d'octets à lire ou écrire
volatile unsigned long  ticks; // compteur système incrémentée à toute les 62,5µSec.
volatile unsigned int video_addr; // addresse de début de la mémoire vidéo dans la SRAM
unsigned char *mcu_addr; // pointeur vers buffer vidéo en RAM
volatile unsigned char page; // page vidéo dans la sram


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
char msg[32]; // contient ligne de texte à afficher

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
	sram_addr=page*PAGE_SIZE;
	byte_count=PAGE_HEIGHT;
	video_op=VID_CLR;
	while (video_op);

}//clear_screen()


void dot(int x, int y, unsigned int pg){
	if ((x>>3)>=BYTES_PER_LINE) return;
	disp_buffer[0] = (1 << (7-(x & 7)));
	sram_addr=pg*PAGE_SIZE+(y<<5)+(x>>3);
	mcu_addr= &disp_buffer[0];
	video_op=VID_SET;
	while (video_op);
} // dot()

void erase_dot(int x, int y, unsigned int pg){
	if ((x>>3)>=BYTES_PER_LINE) return;
	disp_buffer[0] = (1 << (7-(x & 7)));
	sram_addr=pg*PAGE_SIZE*(y<<5)+(x>>3);
	mcu_addr= &disp_buffer[0];
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
void draw_line(int x0, int y0, int x1, int y1, unsigned int pg){
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
      if (steep) dot(y,x,pg); else dot(x,y,pg);
      error = error - dy;
      if (error < 0){
          y = y + ystep;
          error = error + dx;
      }
  }
} // line()


void rectangle(int x0, int y0, int x1, int y1,unsigned int pg){
	draw_line(x0,y0,x1,y0,pg);
	draw_line(x1,y0,x1,y1,pg);
	draw_line(x0,y1,x1,y1,pg);
	draw_line(x0,y0,x0,y1,pg);
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


const char title[]="demo utilisation 2 pages video.";


void print(int x, int y, const char* str,unsigned int pg){
int idx=0;
	// lire mémoire spi qui va contenir la chaîne de caractère
	byte_count=BYTES_PER_LINE;
	mcu_addr=&disp_buffer[0];
	sram_addr=pg*PAGE_SIZE+(y<<5);
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
	sram_addr=pg*PAGE_SIZE+(y<<5);
	mcu_addr=&disp_buffer[0];
	for (idx=0;idx<7;idx++){
		video_op=VID_WRITE;
		while(video_op);
		sram_addr += BYTES_PER_LINE;
		mcu_addr += BYTES_PER_LINE;
	}
}// print()


void clear_lines(unsigned int first, unsigned int last, unsigned int pg){
	byte_count=last-first+1;
	sram_addr=pg*PAGE_SIZE+(first<<5);
	video_op=VID_CLR;
	while (video_op);
} //clear_lines()

void read_lines(unsigned int first, unsigned int last, unsigned int pg, unsigned char *addr){
int i,line_count;
	sram_addr=pg*PAGE_SIZE+(first<<5);
	line_count=last-first+1;
	byte_count=BYTES_PER_LINE;
	mcu_addr=addr;
	for (i=0;i<line_count;i++){
		video_op=VID_READ;
		while (video_op);
		sram_addr += BYTES_PER_LINE;
		mcu_addr += BYTES_PER_LINE;
	}
} // read_lines()

void write_lines(unsigned int first, unsigned int last, unsigned int pg, unsigned char *addr){
	int i,line_count;
		sram_addr=pg*PAGE_SIZE+(first<<5);
		line_count=last-first+1;
		byte_count=BYTES_PER_LINE;
		mcu_addr=addr;
		for (i=0;i<line_count;i++){
			video_op=VID_WRITE;
			while (video_op);
			sram_addr += BYTES_PER_LINE;
			mcu_addr += BYTES_PER_LINE;
		}
}// write_lines()

#define TOP_LINE FIRST_LINE+8
#define BOTTOM_LINE SCREEN_HEIGHT-10

void next_copy(){
unsigned int line;
	line=TOP_LINE;
	while (1){
		read_lines(line,line,page,&disp_buffer[0]);
		write_lines(line,line,1-page,&disp_buffer[0]);
		line++;
		if (line>BOTTOM_LINE)
			break;
	}
} // next_copy()


#include "loup.h"
#define MARGIN ((BYTES_PER_LINE-IMG_WIDTH)>>1)
void copy_img_2_sram(unsigned char pg){//copie l'image qui est dans la flash vers SPI RAM
	unsigned int i,l;
	for (i=0;i<MARGIN;i++){ // efface les marges
		disp_buffer[i]=0;
		disp_buffer[i+MARGIN+IMG_WIDTH]=0;
	}
	mcu_addr=&disp_buffer[0];
	sram_addr=pg*PAGE_SIZE+((FIRST_LINE+8)<<5)+MARGIN;
	byte_count=IMG_WIDTH; //BYTES_PER_LINE-MARGIN;
	for (l=0;l<DISP_LINES-14;l++){
		for (i=0;i<IMG_WIDTH;i++){
			disp_buffer[i]=loup[l][i];
		}
		video_op=VID_WRITE;
		while (video_op);
		sram_addr += BYTES_PER_LINE;
	}
}// copy_2_sram()


/*
 * main.c
 */
void main(void) {
unsigned int copy=0;
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    configure_clock();
    P2DIR = SYNC_OUT;  // P2.1 en sortie
    P2SEL = SYNC_OUT;  // minuterie sur P2.1
    // configuration SPI RAM
    sram_init(SRAM_SEQ_MODE,SRAM_SMCLK, SMCLK_DIV);
    //configuration minuterie 1 pour syncho NTSC
    TA1CCR0=H_LINE;
    TA1CCR1=H_LINE-H_SYNC; // commence avec une synchro verticale
    TA1CCR2=H_DISPLAY_DELAY;
    TA1CCTL1=OUTMOD_3; // mode toggle/set
    TA1CCTL0 = CCIE;
    TA1CCTL2 |= CCIE;
    TA1CTL = TASSEL_2+MC_1; // SMCLK, UP mode
	ln_cnt=1;
	blanked=1;
	_blank_level();
	video_addr=0;
    _enable_interrupts();
    clear_screen();
    page=1;
    clear_screen();
    page=0;
    print((SCREEN_WIDTH-(6*strlen(&title[0])))>>1,FIRST_LINE,&title[0],0);
    print((SCREEN_WIDTH-(6*strlen(&title[0])))>>1,FIRST_LINE,&title[0],1);
    copy_img_2_sram(page);
    while (1){
	    clear_lines(SCREEN_HEIGHT-9,SCREEN_HEIGHT-1,page);
	    sprintf(msg,"copie: %d",copy);
	    print(20,SCREEN_HEIGHT-8,&msg[0],page);
	    next_copy();
	    copy++;
	    page = 1-page;
	    video_addr=page*PAGE_SIZE;
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
