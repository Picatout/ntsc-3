/*
 * main.h
 *
 *  Created on: 2013-07-31
 *      Author: Jacques
 */

#ifndef MAIN_H_
#define MAIN_H_

#define H_LINE 1000 	// 62,5�Sec
#define H_SYNC 75   	// 4,7�Sec
#define H_DISPLAY_DELAY  120
#define FIRST_LINE 6   	// premi�re ligne visible � l'�cran
#define LAST_LINE  235 	// derni�re ligne visible � l'�cran
#define DISP_LINES 230
#define BYTES_PER_LINE 32
#define SCREEN_WIDTH BYTES_PER_LINE*8  	// pixels horizontal
#define SCREEN_HEIGHT 230 	// 230 pixels vertical
#define VIDEO_MEMSIZE (SCREEN_WIDTH>>3)*SCREEN_HEIGHT  // nombre octets m�moire vid�o dans SRAM
#define SMCLK_DIV 3  // diviseur pour SPI lors de l'affichage
#define BLKCLK_DIV 2 // diviseur pour le SPI hors affichage

#define BLK_OUT	BIT0  // P2.2 signal blanking

// op�rations sur m�moire vid�o
#define VID_NONE   0  // aucune op�ration
#define VID_WRITE  1   // �cris une s�rie d'octets
#define VID_READ   2   // lis une s�rie d'octets
#define VID_SET    3   //  met � 1 les bits
#define VID_RST	   4   //  met � 0 les bits
#define VID_INV    5  //  inverse les bits
#define VID_CLR    6  // efface byte_count lignes


#endif /* MAIN_H_ */
