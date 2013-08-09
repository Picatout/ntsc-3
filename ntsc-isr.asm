; NTSC-ISR.ASM
; Description: routines d'interruptions pour
;              la génération des signaux synchronistation et vidéo NTSC
; Auteur: Jacques Deschênes
; Date: 2013-07-31

	.cdecls  C,LIST, "msp430.h", "spi-ram.h", "main.h"

	.text
	.retain			; ne pas oublier
	.retainrefs		; ces 2 directives

	.global ln_cnt, blanked, video_op, byte_count
	.global ticks
	.global video_addr, sram_addr, mcu_addr


VSYNC_START	.equ	264
VSYNC_END	.equ	4
DISP_START  .equ	20
DISP_END	.equ    263


_wait_txifg	.macro				; attend que le bit SRAM_TXIFG vienne à 1 dans IFG2
m1?	bit.b #SRAM_TXIFG, &IFG2	; vérification bit interruption
	jz m1?						; 7 cycles
	.endm

_wait_rxifg .macro    			; attend que le bit SRAM_RXIFG vienne à 1 dans IFG2
m1?: bit.b #SRAM_RXIFG, &IFG2	; 7 cycles
	jz m1?
	.endm

_wait_spi_idle .macro				; attend que le périphérique SPI est terminé l'opération
m1? bit.b #UCBUSY, &SRAM_SPI_STAT	; 7 cycles
	jnz m1?
	.endm


_disable_sram .macro			; désactive la mémoire SPI RAM
	bis.b #SRAM_CS, &CS_OUT		; 5 cycles
	.endm

_enable_sram .macro				; active la mémoire SPI RAM
	bic.b #SRAM_CS, &CS_OUT		; 5 cycles
	.endm

_black_level .macro
	bic.b #BLK_OUT, &P2DIR		; met la broche en mode HiZ.
	.endm						;  5 cycles

_blank_level .macro
	bic.b #BLK_OUT, &P2OUT	; met la broche en mode sortie niveau 0.
	bis.b #BLK_OUT, &P2DIR	; 10 cycles
	.endm

;********************************
; initialisation lecture SPI RAM
;********************************
_init_sram_read .macro				; 17 cycles
	_enable_sram
	mov.b #SRAM_READ, &SRAM_TXBUF
	_wait_txifg
	.endm

;********************************
; initialisation écriture SPI RAM
;********************************
_init_sram_write .macro				; 17 cycles
	_enable_sram
	mov.b #SRAM_WRITE, &SRAM_TXBUF
	_wait_txifg
	.endm
;*********************************
; envoie l'adresse à la SPI RAM
;*********************************
_send_sram_addr .macro				; 24 cycles
	mov.b sram_addr+1, &SRAM_TXBUF
	_wait_txifg
	mov.b sram_addr, &SRAM_TXBUF
	_wait_txifg
	.endm

;*********************************
; service d'interruption TIMER1_A0
;*********************************
ta1_ccr0_isr:
	push R4
	push R5
	inc ticks 			; variable 32 bits
	jnc $1				; incrémentrée à chaque
	inc ticks+1			; interruption.
$1:
	.newblock
	add #1, ln_cnt
	mov ln_cnt,  R4		; switch (ln_cnt)
	cmp #DISP_START, R4
	jeq disp_start
	cmp #DISP_END, R4
	jeq disp_end
	cmp #VSYNC_END, R4
	jeq vsync_end
	cmp #VSYNC_START, R4
	jeq vsync_start
	tst.b blanked
	jeq ta1_ccr0_exit
; opérations vidéo effectuées pendant la phase vsync
	mov.b video_op, R4		; switch(video_op)
	tst.b R4
	jeq ta1_ccr0_exit
	mov.b #BLKCLK_DIV, &SRAM_BR0
	cmp.b #VID_WRITE, R4
	jeq video_write
	cmp.b #VID_READ, R4
	jeq video_read
	cmp.b #VID_CLR, R4
	jeq video_clear
	cmp.b #VID_SET, R4
	jeq video_set
	cmp.b #VID_RST, R4
	jeq video_rst
	cmp.b #VID_INV, R4
	jeq video_inv
	jmp video_op_exit
video_rst: ; *sram_addr &= ~(*mcu_addr)
	_init_sram_read
	_send_sram_addr
	mov.b #0, &SRAM_TXBUF
	_wait_spi_idle
	mov.b &SRAM_RXBUF, R5
	mov mcu_addr, R4
	bic.b 0(R4), R5 ; R5 &= ~(*mcu_addr)
	_disable_sram
	_init_sram_write
	_send_sram_addr
	mov.b R5, &SRAM_TXBUF
	mov #VID_NONE, video_op
	_wait_spi_idle
	_disable_sram
	jmp video_op_exit
video_set: ; *sram_addr  |= *mcu_addr
	_init_sram_read
	_send_sram_addr
	mov.b #0, &SRAM_TXBUF
	_wait_spi_idle
	mov.b &SRAM_RXBUF, R5
	mov mcu_addr, R4
	bis.b 0(R4), R5
	_disable_sram
	_init_sram_write
	_send_sram_addr
	mov.b R5, &SRAM_TXBUF
	mov #VID_NONE, video_op
	_wait_spi_idle
	_disable_sram
	jmp video_op_exit
video_inv: ; SRAM ^= line[0]
	_init_sram_read
	_send_sram_addr
	mov.b #0, &SRAM_TXBUF
	_wait_spi_idle
	mov.b &SRAM_RXBUF, R5
	mov mcu_addr, R4
	xor.b 0(R4), R5
	_disable_sram
	_init_sram_write
	_send_sram_addr
	mov.b R5, &SRAM_TXBUF
	mov #VID_NONE, video_op
	_wait_spi_idle
	_disable_sram
	jmp video_op_exit
video_read: ; lis une série d'octets maximum BYTES_PER_LINE
	mov.w mcu_addr, R5	; addresse du buffer dans R5
	mov.w byte_count, R4 ; nombre d'octets à lire
	_init_sram_read
	_send_sram_addr
	_wait_spi_idle
	mov.b &SRAM_RXBUF,0(R5)
	mov.b #0, &SRAM_TXBUF
	_wait_txifg
$1:	mov.b #0, &SRAM_TXBUF		; 5
	_wait_rxifg					; 7 / boucle
	mov.b &SRAM_RXBUF,0(R5)     ; 6
	inc R5						; 1
	dec R4						; 1
	jnz $1						; 2
	.newblock
	mov #VID_NONE, video_op
	_disable_sram
	jmp video_op_exit
video_write:; écris dans la mémoire SRAM pendant les lignes non affichées, maximum BYTES_PER_LINE
	mov.w mcu_addr, R5	 ; addresse du buffer dans R5
	mov.w byte_count, R4 ; nombre d'octets à écrire
	_init_sram_write
	_send_sram_addr
$1:	mov.b 0(R5), &SRAM_TXBUF  ; 6 cycles
	inc R5					  ; 1
	_wait_txifg
	dec R4					  ; 1
	jnz $1					  ; 2
	.newblock
	mov #VID_NONE, video_op
	_wait_spi_idle
	_disable_sram
	jmp video_op_exit
video_clear:; efface byte_count lignes à partir de l'adressse sram_addr
	mov #BYTES_PER_LINE, R4
	_init_sram_write
	_send_sram_addr
$1: mov.b #0, SRAM_TXBUF
	_wait_txifg
	dec R4
	jnz $1
	.newblock
	_wait_spi_idle
	_disable_sram
	add #BYTES_PER_LINE, &sram_addr
	dec &byte_count
	jnz video_op_exit
	mov.b #VID_NONE, video_op
	jmp video_op_exit
vsync_start:
	mov #1, ln_cnt
	mov #(H_LINE-H_SYNC), &TA1CCR1
	jmp ta1_ccr0_exit
vsync_end:
	mov #H_SYNC, TA1CCR1
	jmp ta1_ccr0_exit
disp_end:
	mov.b #1, blanked
	_disable_sram
	jmp ta1_ccr0_exit
disp_start:
	mov.b #0, blanked
	_init_sram_read
	mov.b video_addr+1, &SRAM_TXBUF
	_wait_txifg
	mov.b video_addr, &SRAM_TXBUF
	_wait_txifg
	jmp ta1_ccr0_exit
video_op_exit:
	mov.b #SMCLK_DIV, &SRAM_BR0
ta1_ccr0_exit:
	pop R5
	pop R4
	reti

;*********************************
; service d'interruption TIMER1_A1
;*********************************
ta1_ccrx_isr:
	push R4  ; variable temporaire test
	push R5	 ; compteur de boucle
	mov.b	 &TA1IV, R4
	cmp #TA1IV_TACCR2, R4
	jnz ta1_ccrx_exit
	tst.b blanked
	jnz ta1_ccrx_exit
$1:	cmp #130, TA1R
	jl $1
	.newblock
	_black_level
	; affichage des bits de la ligne, via lecture SPI RAM
	mov #4, R4
$2: dec R4		; délais avant de débuter l'envoie de bits
	jnz $2
	.newblock
	mov #BYTES_PER_LINE-1, R5
	mov.b #0, &SRAM_TXBUF
	_wait_txifg
$1:	mov.b #0, &SRAM_TXBUF	; 5 cycles
	mov #5, R4	; 2 cycle délais d'attente temps lecture octet
$2:	dec R4		; 1 cycle
	jnz $2		; 2 cycles
;	nop			; 1 cycle
	dec R5		; 1 cycle
	jnz $1		; 2 cycles, total 26
	.newblock
wait_eol: 			; attend la fin de la ligne, TA1R>=950
$1:	cmp #962, TA1R
	jl $1
	.newblock
	_blank_level
ta1_ccrx_exit:
	pop R5
	pop R4
	reti


;****************************
;    vecteurs d'interruption
;****************************
	.sect 	TIMER1_A1_VECTOR
	.short 	ta1_ccrx_isr

	.sect 	TIMER1_A0_VECTOR
	.short 	ta1_ccr0_isr


	.end

