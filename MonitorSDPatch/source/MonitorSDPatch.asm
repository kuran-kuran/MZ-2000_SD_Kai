KNUMBS		EQU		06A2H
GETL		EQU		06A4H
LETLN		EQU		0A2EH
MSGPR		EQU		0889H
GETKEY		EQU		0832H
BREAK		EQU		0562H
PRTBYT		EQU		05F3H
PRNT		EQU		08C6H
DISPCH		EQU		08C6H
DPCT		EQU		08C6H
IBUFE		EQU		1140H
FNAME		EQU		1141H
EADRS		EQU		1152H
FSIZE		EQU		1152H
SADRS		EQU		1154H
EXEAD		EQU		1156H
DSPX		EQU		11D1H
MONITOR		EQU		00B1H
;MZ-80B SB-1520用(SP-5520 MZ-LISP_80B)
;LBUF		EQU		1093H
;MBUF		EQU		109DH
;MZ-2000 MZ-1Z001M用(MZ-1Z001 MZ-1Z002 MZ-LIST_2000)
LBUF		EQU		10ABH
MBUF		EQU		10B6H

;SB-5520: 408Bh
;SB-6520: 4DFBh
;MZ-1Z001: 409Ah
;MZ-1Z002: 4120h
;MZ-1Z003: 3E69h
;MZ-2Z001: 4F57h
;MZ-2Z002: 4F7Ah
;MZ-2Z021: 409Ah
;SuperColorBASIC: 409Ah
;KANJIBASIC SB-2000T: 4F57h
BASBUF		EQU		409AH

MSG_FNAME	EQU		MSG99
MSG_F0		EQU		MSG99
MSG_CMD		EQU		MSG99

;0A0H PORTA 送信データ(下位4ビット)
;0A1H PORTB 受信データ(8ビット)
;
;0A2H PORTC Bit
;7 IN  CHK
;6 IN
;5 IN
;4 IN 
;3 OUT
;2 OUT FLG
;1 OUT
;0 OUT
;
;0A3H コントロールレジスタ

ENT:
		ORG		00251H    ;BASIC MZ-1Z001M

;******************** MONITOR CMTルーチン代替 *************************************
; #0251h ?WRI
ENT1:	DI
		JP		MSHED

		
;**** 8255初期化 ****
;PORTC下位BITをOUTPUT、上位BITをINPUT、PORTBをINPUT、PORTAをOUTPUT
INIT:	LD		A,8AH
		OUT		(0A3H),A
;出力BITをリセット
INIT2:	LD		A,00H      ;PORTA <- 0
		OUT		(0A0H),A
		OUT		(0A2H),A   ;PORTC <- 0
		RET

;データ受信
DBRCV:	LD		DE,(FSIZE)
		LD		HL,(SADRS)
		JP		DBRCV0

; #026Ah WRI2
		NOP

DBRCV0:
DBRLOP:	CALL	RCVBYTE
		LD		(HL),A
		DEC		DE
		LD		A,D
		OR		E
		INC		HL
		JR		NZ,DBRLOP   ;DE=0までLOOP
		RET

STSV2:                      ;ファイルネームの取得に失敗
		LD		DE,MSG_FNAME
		JR		ERRMSG

SVER0:
		POP		DE         ;CALL元STACKを破棄する
SVERR:
		CP		0F0H
		JR		NZ,ERR3
		JR		SVERR0

; #0282h ?WRD
ENT2:	DI
		JP		MSDAT

SVERR0:
		LD		DE,MSG_F0  ;SD-CARD INITIALIZE ERROR
		JR		ERRMSG

		NOP
		NOP
		NOP
; #028Eh ?RDI
ENT3:	DI
		JP		MLHED

ERR3:	CP		0F1H
		JR		NZ,ERR4
		LD		DE,MSG_F1  ;NOT FIND FILE
		JR		ERRMSG
ERR4:	CP		0F3H
		JR		NZ,ERR5
		LD		DE,MSG_F3  ;FILE EXIST
		JR		ERRMSG
ERR5:	CP		0F4H
		JR		NZ,ERR99
		LD		DE,MSG_CMD
		JR		ERRMSG
ERR99:	CALL	PRTBYT
		JR		ERR99_2

; #02B2h ?RDD
ENT4:	DI
		JP		MLDAT

ERR99_2:
		CALL	PRNT
		LD		DE,MSG99   ;その他ERROR

		NOP
		NOP
; #02BEh ?VRFY
ENT5:	DI
		JP		MVRFY

ERRMSG:	CALL	MSGPR
		CALL	LETLN
MON:	JP		MONITOR

;ヘッダ送信
HDSEND:	PUSH	HL
		LD		B,20H
SS1:	LD		A,(HL)     ;FNAME送信
		CALL	SNDBYTE
		INC		HL
		DEC		B
		JR		NZ,SS1
		LD		A,0DH
		CALL	SNDBYTE
		POP		HL
		LD		B,10H
SS2:	LD		A,(HL)     ;PNAME送信
		CALL	SNDBYTE
		INC		HL
		DEC		B
		JR		NZ,SS2
		LD		A,0DH
		CALL	SNDBYTE
		LD		HL,SADRS   ;SADRS送信
		LD		A,(HL)
		CALL	SNDBYTE
		INC		HL
		LD		A,(HL)
		CALL	SNDBYTE
		LD		HL,EADRS   ;EADRS送信
		LD		A,(HL)
		CALL	SNDBYTE
		INC		HL
		LD		A,(HL)
		CALL	SNDBYTE
		LD		HL,EXEAD   ;EXEAD送信
		LD		A,(HL)
		CALL	SNDBYTE
		INC		HL
		LD		A,(HL)
		JP	SNDBYTE

;データ送信
;SADRSからEADRSまでを送信
DBSEND:	LD		HL,(EADRS)
		EX		DE,HL
		LD		HL,(SADRS)
DBSLOP:	LD		A,(HL)
		CALL	SNDBYTE
		LD		A,H
		CP		D
		JR		NZ,DBSLP1
		LD		A,L
		CP		E
		JR		Z,DBSLP2   ;HL = DE までLOOP
DBSLP1:	INC		HL
		JR		DBSLOP
DBSLP2:	RET

;****** FILE NAME 取得 (IN:DE コマンド文字の次の文字 OUT:HL ファイルネームの先頭)*********
STFN:	PUSH	AF
STFN1:	INC		DE         ;ファイルネームまでスペース読み飛ばし
		LD		A,(DE)
		CP		20H
		JR		Z,STFN1
		CP		30H        ;「0」以上の文字でなければエラーとする
		JP		C,STSV2
		EX		DE,HL
		POP		AF
		RET

;**** コマンド送信 (IN:A コマンドコード)****
STCD:	CALL	SNDBYTE    ;Aレジスタのコマンドコードを送信
		JP		RCVBYTE    ;状態取得(00H=OK)

;**** ファイルネーム送信(IN:HL ファイルネームの先頭) ******
STFS:	LD		B,20H
STFS1:	LD		A,(HL)     ;FNAME送信
		CALL	SNDBYTE
		INC		HL
		DEC		B
		JR		NZ,STFS1
		LD		A,0DH
		CALL	SNDBYTE
		JP		RCVBYTE    ;状態取得(00H=OK)

;**** コマンド、ファイル名送信 (IN:A コマンドコード HL:ファイルネームの先頭)****
STCMD:	CALL	STFN       ;ファイルネーム取得
		PUSH	HL
		CALL	STCD       ;コマンドコード送信
		POP		HL
		AND		A          ;00以外ならERROR
		JP		NZ,SVER0
		CALL	STFS       ;ファイルネーム送信
		AND		A          ;00以外ならERROR
		JP		NZ,SVER0
		RET

;*********************** 0436H MONITOR ライト インフォメーション代替処理 ************
MSHED:
		PUSH	DE
		PUSH	BC
		PUSH	HL
		CALL	INIT
		LD		A,91H      ;HEADER SAVEコマンド91H
		CALL	MCMD       ;コマンドコード送信
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

;S-OS SWORD、8080用テキスト・エディタ＆アセンブラはファイルネームの後ろが20h詰めとなるため0dhに修正
		LD		B,11H
		LD		HL,FNAME+10H     ;ファイルネーム
		LD		A,0DH            ;17文字目には常に0DHをセットする
		LD		(HL),A
MSH0:	LD		A,(HL)
		CP		0DH              ;0DHであればひとつ前の文字の検査に移る
		JR		Z,MSH1
		CP		20H              ;20Hであれば0DHをセットしてひとつ前の文字の検査に移る
		JR		NZ,MSH2          ;0DH、20H以外の文字であれば終了
		LD		A,0DH
		LD		(HL),A
		
MSH1:	DEC		HL
		DEC		B
		JR		NZ,MSH0

MSH2:	CALL	LETLN
		LD		DE,WRMSG   ;"WRITING "
		CALL	MSGPR        ;メッセージ表示
		LD		DE,FNAME     ;ファイルネーム
		CALL	MSGPR       ;メッセージ表示

		LD		HL,IBUFE
		LD		B,80H
MSH3:	LD		A,(HL)     ;インフォメーション ブロック送信
		CALL	SNDBYTE
		INC		HL
		DEC		B
		JR		NZ,MSH3

		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		JP		MRET       ;正常RETURN

;******************** 0475H MONITOR ライト データ代替処理 **********************
MSDAT:
		PUSH	DE
		PUSH	BC
		PUSH	HL
		LD		A,92H      ;DATA SAVEコマンド92H
		CALL	MCMD       ;コマンドコード送信
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		LD		HL,FSIZE   ;FSIZE送信
		LD		A,(HL)
		CALL	SNDBYTE
		INC		HL
		LD		A,(HL)
		CALL	SNDBYTE

		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		LD		DE,(FSIZE)
		LD		HL,(SADRS)
MSD1:	LD		A,(HL)
		CALL	SNDBYTE      ;SADRSからFSIZE Byteを送信。分割セーブの場合、直前に0436HでOPENされたファイルを対象として256バイトずつ0475HがCALLされる。
		DEC		DE
		LD		A,D
		OR		E
		INC		HL
		JR		NZ,MSD1
		
		JP		MRET       ;正常RETURN

;************************** 04D8H MONITOR リード インフォメーション代替処理 *****************
MLHED:
		PUSH	DE
		PUSH	BC
		PUSH	HL
		CALL	INIT

		; BASBUFチェック
		LD		HL, BASBUF
		LD		A, (HL)
		CP		0DH
		JR		Z, MLH3 ; 0Dhの場合はBASICのファイル名は無効なのでモニタからのロードへ
		OR		A
		JR		Z, MLH3 ; 00hの場合はBASICのファイル名は無効なのでモニタからのロードへ
		; BASICからのロードなのでBASICで指定されたファイル名をコピーする、そしてBASICのファイル名を無効にする
		LD		DE, LBUF + 10
MLH0:
		LD		A, (HL)
		LD		(DE), A
		INC		DE
		LD		(HL), 0DH
		INC		HL
		CP		0DH
		JR		NZ, MLH0
		XOR		A
		LD		(DE), A
MLH3:
		; ロード
		LD		DE, LBUF + 10

		LD		A,93H      ;HEADER LOADコマンド93H
		CALL	MCMD       ;コマンドコード送信
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

MLH1:
		LD		A,(DE)
		CP		20H                 ;行頭のスペースをファイルネームまで読み飛ばし
		JR		NZ,MLH2
		INC		DE
		JR		MLH1
MLH2:	LD		B,20H
MLH4:	LD		A,(DE)     ;FNAME送信
		CALL	SNDBYTE
		INC		DE
		DEC		B
		JR		NZ,MLH4
		LD		A,0DH
		CALL	SNDBYTE
		
		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		LD		HL,IBUFE
		LD		B,80H
MLH5:	CALL	RCVBYTE    ;読みだされたインフォメーションブロックを受信
		LD		(HL),A
		INC		HL
		DEC		B
		JR		NZ,MLH5

		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		JP		MRET       ;正常RETURN

;******* アプリケーション内SD-CARD操作処理用ERROR処理 **************
SERR:
		CP		0F0H
		JR		NZ,SERR3
		LD		DE,MSG_F0
		JR		SERRMSG
		
SERR3:	CP		0F1H
		JR		NZ,SERR99
		LD		DE,MSG_F1
		JR		SERRMSG
		
SERR99:	CALL	PRTBYT
		LD		DE,MSG99
		
SERRMSG:
		CALL	MSGPR
		CALL	LETLN
		POP		BC
		POP		DE
		POP		HL
;**** ファイルネーム入力へ復帰 ****
		JP		MERR ;MLH6

;**** コマンド文字列比較 ****
CMPSTR:
		PUSH	BC
		PUSH	DE
CMP1:	LD		A,(DE)
		CP		(HL)
		JR		NZ,CMP2
		DEC		B
		JR		Z,CMP2
		CP		0Dh
		JR		Z,CMP2
		INC		DE
		INC		HL
		JR		CMP1
CMP2:	POP		DE
		POP		BC
		RET

;**** コマンドリスト ****
; 将来拡張用
;CMD1:	DB		"FDL",0DH


;**************************** 04F8H MONITOR リード データ代替処理 ********************
MLDAT:
		PUSH	DE
		PUSH	BC
		PUSH	HL
		LD		A,94H      ;DATA LOADコマンド94H
		CALL	MCMD       ;コマンドコード送信
		AND		A          ;00以外ならERROR
		JR		MLDAT0

; 048C OPEN:
		RET

MLDAT0:
		JP		NZ,MERR
		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR

		LD		DE,FSIZE   ;FSIZE送信
		LD		A,(DE)
		CALL	SNDBYTE
		INC		DE
		LD		A,(DE)
		CALL	SNDBYTE
		CALL	DBRCV      ;FSIZE分のデータを受信し、SADRSから格納。分割ロードの場合、直前に0436HでOPENされたファイルを対象として256バイトずつSADRSが加算されて04F8HがCALLされる。

		JR		MLDAT1

		NOP
		NOP
; #04B1 SERSP
		CALL	SERSP
SERSP:
		RET

MLDAT1:
		CALL	RCVBYTE    ;状態取得(00H=OK)
		AND		A          ;00以外ならERROR
		JP		NZ,MERR
		JR		MRET       ;正常RETURN

;************************** 0588H VRFY CMT ベリファイ代替処理 *******************
MVRFY:	XOR		A          ;正常終了フラグ

		RET

;******* 代替処理用コマンドコード送信 (IN:A コマンドコード) **********
MCMD:
		CALL	SNDBYTE    ;コマンドコード送信
		JP	RCVBYTE    ;状態取得(00H=OK)

;****** 代替処理用正常RETURN処理 **********
MRET:	POP		HL
		POP		BC
		POP		DE
		XOR		A          ;正常終了フラグ
		
		RET

;******* 代替処理用ERROR処理 **************

		NOP
		NOP
		NOP
; #04CE MSTOP
		RET

MERR:
		CP		0F0H
		JR		NZ,MERR3
		LD		DE,MSG_F0
		JR		MERRMSG

		NOP
		NOP
; #04DA BLK3
		RET

MERR3:	CP		0F1H
		JR		NZ,MERR99
		LD		DE,MSG_F1
		JR		MERRMSG

MERR99:	CALL	PRTBYT
		LD		DE,MSG99
		
MERRMSG:
		CALL	MSGPR
		CALL	LETLN
		POP		HL
		POP		BC
		POP		DE
		LD		A,02H
		SCF

		RET

		NOP
		NOP
; #04F9 TSPE
		RET

;**** 1BYTE受信 ****
;受信DATAをAレジスタにセットしてリターン
RCVBYTE:
		CALL	F1CHK      ;PORTC BIT7が1になるまでLOOP
		IN		A,(0A1H)   ;PORTB -> A
		PUSH 	AF
		LD		A,05H
		OUT		(0A3H),A    ;PORTC BIT2 <- 1
		CALL	F2CHK      ;PORTC BIT7が0になるまでLOOP
		LD		A,04H
		OUT		(0A3H),A    ;PORTC BIT2 <- 0
		POP 	AF
		RET
		
;**** 1BYTE送信 ****
;Aレジスタの内容をPORTA下位4BITに4BITずつ送信
SNDBYTE:
		PUSH	AF
		RRA
		JR		SNDBYTE0

; #0511 DEL6
		RET

SNDBYTE0:
		RRA
		RRA
		RRA
		JR		SNDBYTE1

; 0517 DEL1M
		RET

SNDBYTE1:
		AND		0FH
		CALL	SND4BIT
		POP		AF
		AND		0FH
		JR	SND4BIT

;**** 4BIT送信 ****
;Aレジスタ下位4ビットを送信する
SND4BIT:
		OUT		(0A0H),A
		LD		A,05H
		OUT		(0A3H),A    ;PORTC BIT2 <- 1
		CALL	F1CHK      ;PORTC BIT7が1になるまでLOOP
		LD		A,04H
		OUT		(0A3H),A    ;PORTC BIT2 <- 0
;		JR	F2CHK

;**** BUSYをCHECK(0) ****
; 82H BIT7が0になるまでLOOP
F2CHK:	IN		A,(0A2H)
		AND		80H        ;PORTC BIT7 = 0?
		JR		NZ,F2CHK
		RET

;**** BUSYをCHECK(1) ****
; 82H BIT7が1になるまでLOP
F1CHK:	IN		A,(0A2H)
		AND		80H        ;PORTC BIT7 = 1?
		JR		Z,F1CHK
		RET

;******** MESSAGE DATA ********************
WRMSG:
		DB		"WRITING "
		DB		0DH

MSG_F1:
		DB		"NO FILE"
		DB		0DH
MSG_F3:
		DB		"EXISTS"
		DB		0DH
MSG99:
		DB		" ERR"
		DB		0DH

TAIL:
TAIL_RESERVED	EQU	0562h - TAIL
		DS	TAIL_RESERVED

		END	000B1h
