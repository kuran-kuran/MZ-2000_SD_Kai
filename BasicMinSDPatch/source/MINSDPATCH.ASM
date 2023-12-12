; MZ-2000/80BのBASIC用mz2000_sd読み込みルーチン

;	ORG 04C62h ; MZ-1Z001
;	ORG 05D22h ; MZ-1Z002
; 	ORG 049EEh ; SB-5520

; 多分MZ2000_SDから起動しているので初期化は必要ないに違いない
;**** 8255初期化 ****
	;PORTC下位BITをOUTPUT、上位BITをINPUT、PORTBをINPUT、PORTAをOUTPUT
;INIT_2000SD:
;	LD	A, 8Ah
;	OUT	(0A3h), A
;	;出力BITをリセット
;INIT2:	LD	A, 00h      ;PORTA <- 0
;	OUT	(0A0h), A
;	OUT	(0A2h), A   ;PORTC <- 0
;	RET

;**** 通常ファイル読み込み ****
; 通常ファイル読み込み
; DE: SDファイル名
SD_LOAD:
	LD	A, 81h       ;LOADコマンド81h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00h=OK)
;	OR	A
;	JR	NZ, SD_LOAD1

	;ファイル名送信
	; DE: SDファイル名 (33bytes)
	LD	B, 33
SENDFILENAME1:
	LD	A, (DE)
	INC	DE
	OR	A
	JR	Z, SENDFILENAME2
	CALL	SNDBYTE
	DEC	B
	JR	NZ, SENDFILENAME1
	JR	SENDFILENAME3
SENDFILENAME2:
	; ここに来た時は必ずA=0になっている
	CALL	SNDBYTE
	DEC	B
	JR	NZ, SENDFILENAME2
SENDFILENAME3:
	CALL	RCVBYTE      ;状態取得(00H=OK)
	OR	A
	JR	NZ, SD_LOAD1

	;ファイル読み込みヘッダ受信
	;Result: MZファイル名 (17bytes)
	;        読み込みアドレス (2bytes)
	;        データサイズ (2bytes)
	;        実行アドレス (2bytes)
HDRCV:	LD	HL, 01141h
	LD	B, 11h
HDRC1:	CALL	RCVBYTE     ;MZファイル名受信 (17bytes)
	LD	(HL), A
	INC	HL
	DEC	B
	JR	NZ, HDRC1
	;SADRS取得
	CALL	RCVBYTE
	LD	L, A
;	LD	(HL), A
;	INC	HL
	CALL	RCVBYTE
	LD	H, A
;	LD	(HL), A
;	INC	HL
	;FSIZE取得
	CALL	RCVBYTE
	LD	E, A
;	LD	(HL), A
;	INC	HL
	CALL	RCVBYTE
	LD	D, A
;	LD	(HL), A
;	INC	HL
	;EXEAD取得
	CALL	RCVBYTE
;	LD	(HL), A
;	INC	HL
	CALL	RCVBYTE
;	LD	(HL), A

	;データ受信
	;Result: データ (FSIZE bytes)
	;        ステータス 0xFE:次のデータがある, 0x00次のデータが無い(終了) (1bytes)
DBRCV:	;LD	DE, (FSIZE)
	;LD	HL, (SADRS)

DBRLOP:	CALL	RCVBYTE
	LD	(HL), A
	DEC	DE
	LD	A, D
	OR	E
	INC	HL
	JR	NZ, DBRLOP   ;DE=0までLOOP

SD_LOAD1:
	RET


;**** 1BYTE受信 ****
;受信DATAをAレジスタにセットしてリターン
RCVBYTE:
	CALL	F1CHK ; PORTC BIT7が1になるまでLOOP
	IN	A, (0A1h) ; PORTB -> A
	PUSH 	AF
	LD	A, 05h
	OUT	(0A3h), A ; PORTC BIT2 <- 1
	CALL	F2CHK ; PORTC BIT7が0になるまでLOOP
	LD	A, 04h
	OUT	(0A3h), A ; PORTC BIT2 <- 0
	POP 	AF
	RET

;**** 1BYTE送信 ****
;Aレジスタの内容をPORTA下位4BITに4BITずつ送信
SNDBYTE:
	PUSH	AF
	RRA
	RRA
	RRA
	RRA
	AND	0Fh
	CALL	SND4BIT
	POP	AF
	AND	0Fh
	CALL	SND4BIT
	RET

;**** 4BIT送信 ****
;Aレジスタ下位4ビットを送信する
SND4BIT:
	OUT	(0A0h), A
	LD	A,05h
	OUT	(0A3h), A ; PORTC BIT2 <- 1
	CALL	F1CHK ; PORTC BIT7が1になるまでLOOP
	LD	A,04h
	OUT	(0A3h), A ; PORTC BIT2 <- 0
	CALL	F2CHK
	RET

;**** BUSYをCHECK(1) ****
; 82H BIT7が1になるまでLOP
F1CHK:
	IN	A, (0A2h)
	AND	80h ; PORTC BIT7 = 1?
	JR	Z, F1CHK
	RET

;**** BUSYをCHECK(0) ****
; 82H BIT7が0になるまでLOOP
F2CHK:
	IN	A, (0A2h)
	AND	80h ; PORTC BIT7 = 0?
	JR	NZ, F2CHK
	RET

	END 000B1h
