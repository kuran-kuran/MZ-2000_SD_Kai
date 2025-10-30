;**** 8255初期化 ****
	;PORTC下位BITをOUTPUT、上位BITをINPUT、PORTBをINPUT、PORTAをOUTPUT
INIT_2000SD:	LD	A, 8Ah
	OUT	(0A3h), A
	;出力BITをリセット
INIT2:	LD	A, 00h      ;PORTA <- 0
	OUT	(0A0h), A
	OUT	(0A2h), A   ;PORTC <- 0
	RET

; BASICからの通常ファイル読み込み
; USR($xxxx, F$)
; DE: 文字列のアドレス
; BC: 文字列の長さ
SD_USR_LOAD:
	LD	H, D
	LD	L, E
	CALL	SD_LOAD
	RET

SD_USR_OPEN_CONCAT:
	LD	H, D
	LD	L, E
	CALL	SD_OPEN_CONCAT
	RET

SD_USR_READ_CONCAT_1BLOCK:
	CALL	SD_READ_CONCAT_1BLOCK
	RET

SD_USR_FIND_CONCAT_1BLOCK:
	LD	H, D
	LD	L, E
	CALL	SD_FIND_CONCAT_1BLOCK
	RET

SD_USR_NEXT_FILE_EXIST:
	CALL	SD_NEXT_FILE_EXIST
	; 文字列の1バイト目にレスポンスを返す
	LD	(DE), A
	RET

;**** 通常ファイル読み込み ****
; 通常ファイル読み込み
; HL: SDファイル名
; Result Cf: 0:正常, 1:エラー
SD_LOAD:
	LD	A, 81h       ;LOADコマンド81h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00h=OK)
	OR	A
	JR	NZ, SD_LOAD1
	CALL	SENDFILENAME ;ファイル名送信
	OR	A
	JR	NZ, SD_LOAD1
	CALL	HDRCV        ;ヘッダ情報受信
	CALL	DBRCV        ;データ受信
	RET
SD_LOAD1:
	SCF
	RET

;**** 連結ファイル読み込み ****
; 連結ファイルオープン
; HL: SDファイル名
; Result Cf: 0:正常, 1:エラー
SD_OPEN_CONCAT:
	LD	A, 0E0h      ;連結ファイルオープンコマンドE0h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00H=OK)
	OR	A
	JR	NZ, SD_OPEN_CONCAT1
	CALL	SENDFILENAME ;ファイル名送信
	OR	A
	JR	NZ, SD_OPEN_CONCAT1
	RET
SD_OPEN_CONCAT1:
	SCF
	RET

;**** 連結ファイル1ブロック読み込み ****
SD_READ_CONCAT_1BLOCK:
	LD	A, 0E1h      ;連結ファイル1ブロック読み込みコマンドE1h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00H=OK)
	OR	A
	JR	NZ, SD_READ_CONCAT_1BLOCK1
	CALL	HDRCV        ;ヘッダ情報受信
	CALL	DBRCV        ;データ受信
	CALL	RCVBYTE
	LD	(STATUS), A  ;ステータス: FEh:次のデータがある, 00h: 次のデータが無い(終了)
	RET
SD_READ_CONCAT_1BLOCK1:
	XOR	A
	LD	(STATUS), A
	SCF
	RET

;**** 連結ファイル1ブロックスキップ ****
SD_SKIP_CONCAT_1BLOCK:
	LD	A, 0E2h      ;連結ファイル1ブロックスキップコマンドE2h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00H=OK)
	OR	A
	JR	NZ, SD_SKIP_CONCAT_1BLOCK1
	CALL	RCVBYTE
	LD	(STATUS), A  ;ステータス: FEh:次のデータがある, 00h: 次のデータが無い(終了)
	RET
SD_SKIP_CONCAT_1BLOCK1:
	XOR	A
	LD	(STATUS), A
	SCF
	RET

;**** 連結ファイル検索 ****
SD_FIND_CONCAT_1BLOCK:
	LD	A, 0E3h      ;連結ファイルオープンコマンドE3h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00H=OK)
	OR	A
	JR	NZ, SD_FIND_CONCAT_1BLOCK1
	CALL	SENDMZFILENAME ;MZファイル名送信
	CALL	RCVBYTE
	LD	(STATUS), A  ;ステータス: 00h: 見つかった, F1h:見つからなかった
	RET
SD_FIND_CONCAT_1BLOCK1:
	LD	A, 0FFh
	LD	(STATUS), A
	SCF
	RET

;**** 連結ファイル先頭へシーク ****
SD_TOP_CONCAT:
	LD	A, 0E4h      ;連結ファイル先頭へシークコマンドE4h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00h:OK)
	OR	A
	JR	NZ, SD_TOP_CONCAT1
	CALL	RCVBYTE      ;状態取得(00h:OK, 0FFh:エラー)
	OR	A
	JR	NZ, SD_CLOSE_CONCAT1
	RET
SD_TOP_CONCAT1:
	SCF
	RET

;**** 連結ファイルクローズ ****
SD_CLOSE_CONCAT:
	LD	A, 0E5h      ;連結ファイルクローズE5h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00h:OK)
	OR	A
	JR	NZ, SD_CLOSE_CONCAT1
	CALL	RCVBYTE      ;状態取得(00h:OK, 0FFh:エラー)
	OR	A
	JR	NZ, SD_CLOSE_CONCAT1
	RET
SD_CLOSE_CONCAT1:
	SCF
	RET

;**** 連結ファイルの次のデータがあるか ****
; 文字列の1バイト目にレスポンスを返す
; Result A: 0xFE:次のデータがある, 0x00:次のデータが無い(終了) 0xFF:オープンしていない
SD_NEXT_FILE_EXIST:
	LD	A, 0E6h      ;連結ファイルの次のデータがあるかE6h
	CALL	SNDBYTE
	CALL	RCVBYTE      ;状態取得(00h:OK)
	OR	A
	JR	NZ, SD_NEXT_FILE_EXIST2
	CALL	RCVBYTE      ;状態取得(00h:OK, 0FFh:エラー)
	OR	A
SD_NEXT_FILE_EXIST1:
	RET
SD_NEXT_FILE_EXIST2:
	LD	A, 0FFh
	SCF
	JR	SD_NEXT_FILE_EXIST1

;MZファイル名送信
; HL: MZファイル名 (17bytes)
SENDMZFILENAME:
	LD	B, 17
	JR	SENDFILENAME1

;ファイル名送信
; HL: SDファイル名 (33bytes)
SENDFILENAME:
	LD	B, 33
SENDFILENAME1:
	LD	A, (HL)
	INC	HL
	OR	A
	JR	Z, SENDFILENAME2
	CALL	SNDBYTE
	DEC	B
	JR	NZ, SENDFILENAME1
	JR	SENDFILENAME3
SENDFILENAME2:
	XOR	A
	CALL	SNDBYTE
	DEC	B
	JR	NZ, SENDFILENAME2
SENDFILENAME3:
	CALL	RCVBYTE      ;状態取得(00H=OK)
	RET

;ファイル読み込みヘッダ受信
;Result: MZファイル名 (17bytes)
;        読み込みアドレス (2bytes)
;        データサイズ (2bytes)
;        実行アドレス (2bytes)
HDRCV:	LD	HL, FILEINFO
	LD	B, 11h
HDRC1:	CALL	RCVBYTE     ;MZファイル名受信 (17bytes)
	LD	(HL), A
	INC	HL
	DEC	B
	JR	NZ, HDRC1
	;SADRS取得
	CALL	RCVBYTE
	LD	(HL), A
	INC	HL
	CALL	RCVBYTE
	LD	(HL), A
	INC	HL

;	PUSH	HL
;	LD	HL, (SADRS)
;	CALL	PRINT_HEX16
;	CALL	NEW_LINE
;	POP	HL

	;FSIZE取得
	CALL	RCVBYTE
	LD	(HL), A
	INC	HL
	CALL	RCVBYTE
	LD	(HL), A
	INC	HL

;	PUSH	HL
;	LD	HL, (FSIZE)
;	CALL	PRINT_HEX16
;	CALL	NEW_LINE
;	POP	HL

	;EXEAD取得
	CALL	RCVBYTE
	LD	(HL), A
	INC	HL
	CALL	RCVBYTE
	LD	(HL), A

;	PUSH	HL
;	LD	HL, (EXEAD)
;	CALL	PRINT_HEX16
;	CALL	NEW_LINE
;	POP	HL

	RET

;データ受信
;Result: データ (FSIZE bytes)
DBRCV:	LD	DE, (FSIZE)
	LD	A, D
	OR	E
	RET	Z
	LD	HL, (SADRS)
DBRLOP:	CALL	RCVBYTE
	LD	(HL), A
	DEC	DE
	LD	A, D
	OR	E
	INC	HL
	JR	NZ, DBRLOP   ;DE=0までLOOP
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
; 82H BIT7が1になるまでLOOP getChk(1)
F1CHK:
	IN	A, (0A2h)
	AND	80h ; PORTC BIT7 = 1?
	JR	Z, F1CHK
	RET

;**** BUSYをCHECK(0) ****
; 82H BIT7が0になるまでLOOP getChk(0)
F2CHK:
	IN	A, (0A2h)
	AND	80h ; PORTC BIT7 = 0?
	JR	NZ, F2CHK
	RET

; ファイル情報
FILEINFO:
FNAME:
	DS	17
SADRS:
	DW	0
FSIZE:
	DW	0
EXEAD:
	DW	0
STATUS:
	DB      0
