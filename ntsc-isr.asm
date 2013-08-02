; NTSC-ISR.ASM
; Description: routines d'interruptions pour
;              la génération des signaux synchronistation et vidéo NTSC
; Auteur: Jacques Deschênes
; Date: 2013-07-31

	.cdecls  C,LIST, "msp430.h", "spi-ram.h", "main.h"

	.text
	.retain			; ne pas oublier
	.retainrefs		; ces 2 directives

	.global ln_cnt, display, blanked

_wait_txifg	.macro				; attend que le bit SRAM_TXIFG vienne à 1 dans IFG2
m1?	mov.b &IFG2, R4				; vérification bit interruption
	and #SRAM_TXIFG, R4			; si bit à 1 buffer TX vide
	jz m1?
	.endm

_disable_sram .macro
	bis.b #SRAM_CS, &CS_OUT
	.endm

_enable_sram .macro
	bic.b #SRAM_CS, &CS_OUT
	.endm

_black_level .macro
	bic.b #BLK_OUT, &P2DIR	; met la broche en mode HiZ.
	.endm

_blank_level .macro
	bic.b #BLK_OUT, &P2OUT	; met la broche en mode sortie
	bis.b #BLK_OUT, &P2DIR	; niveau 0.
	.endm


;*********************************
; service d'interruption TIMER1_A0
;*********************************
ta1_ccr0_isr:
	push R4
	add #1, ln_cnt
	mov ln_cnt,  R4
	cmp #20, R4
	jz line_20
	cmp #4, R4
	jz line_4
	cmp #FIRST_LINE, R4
	jz first_line
	cmp #LAST_LINE, R4
	jz last_line
	cmp #263, R4
	jz line_263
	jmp ta1_ccr0_exit
line_263:
	mov #1, ln_cnt
	mov.b #1, blanked
	mov #(H_LINE-H_SYNC), &TA1CCR1
	jmp ta1_ccr0_exit
last_line:
	mov.b #0, display
	_disable_sram
	jmp ta1_ccr0_exit
first_line:
	mov.b #1, display
	jmp ta1_ccr0_exit;
line_4:
	mov #H_SYNC, TA1CCR1
	jmp ta1_ccr0_exit
line_20:
	mov.b #0, blanked
	_enable_sram
	mov.b #SRAM_READ, &SRAM_TXBUF ; initialisaton commande READ
	_wait_txifg
	mov.b #0, &SRAM_TXBUF			; débute la lecture à l'adresse 0, envoie octet fort
	_wait_txifg
	mov.b #0, &SRAM_TXBUF			; envoie octet faible de l'adresse
	_wait_txifg
ta1_ccr0_exit:
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
	_black_level
	tst.b display
	jz	wait_eol
	; affichage des bits de la ligne, via lecture SPI RAM
	mov #BYTES_PER_LINE, R5
$1:	mov.b #0, SRAM_TXBUF	; transmet un 0 pour déclenché la lecture de l'octet
	_wait_txifg
	dec R5
	jnz $1
	.newblock
wait_eol: 			; attend la fin de la ligne, TA1R>=950
$1:	cmp #950, TA1R
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

