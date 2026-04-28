DATA	EQU	09F80h

;	ORG	0B800h
	; 0B800h : 文字数取得
;	JP	GET_LENGTH_BASIC
;	JP	DRAWFONT_ALL_BASIC
;	JP	DRAWFONT_BASIC
;	JP	ERASE_BASIC

; メッセージの長さ取得
; DE: 文字列のアドレス
; BC: 文字列の長さ
; C$=CHR$(ID)
; USR($B800,C$)
; Result: C$の1文字目に文字列の長さ
GET_LENGTH_BASIC PROC
	LD	A, (DE) ; ID
	CALL	GET_LENGTH
	LD	(DE), A
	RET
ENDP

; メッセージをすべて表示する
; DE: 文字列のアドレス
; BC: 文字列の長さ
; C$=CHR$(X)+CHR$(Y)+CHR(C)+CHR$(R)+CHR$(ID)
; USR($B803,C$)
DRAWFONT_ALL_BASIC PROC
	; G-VRAM状態を保存
	IN	A, (0E8h)
	LD	(BACKUP1_GVRAM_STATUS), A

	; G-VRAM有効
	AND	03Fh
	OR	080h
	OUT	(0E8h), A

	LD	A, (DE) ; X
	LD	(DRAW_ALL_X), A
	INC	DE
	LD	A, (DE) ; Y
	LD	(DRAW_ALL_Y), A
	INC	DE
	LD	A, (DE) ; C
	LD	(DRAW_ALL_COLOR), A
	INC	DE
	LD	A, (DE) ; R
	LD	(DRAW_ALL_NEWLINE), A
	INC	DE
	LD	A, (DE) ; ID
	LD	(DRAW_ALL_ID), A

	INC	BC
	LD	DE, DRAW_ALL_WORK
	CALL	GET_LENGTH
	LD	L, A
	XOR	A
@@:
	LD	(DRAW_ALL_INDEX), A
	PUSH	AF
	PUSH	BC
	PUSH	DE
	PUSH	HL
	CALL	DRAWFONT_BASIC
	POP	HL
	POP	DE
	POP	BC
	POP	AF
	INC	A
	DEC	L
	JR	NZ, @b

	; GVRAM状態を元に戻す
	LD	A, (BACKUP1_GVRAM_STATUS)
	OUT	(0E8h), A
	RET
ENDP

; メッセージを1文字ずつ表示する
; DE: 文字列のアドレス
; BC: 文字列の長さ
; C$=CHR$(X)+CHR$(Y)+CHR(C)+CHR$(R)+CHR$(ID)+CHR$(I)
; USR($B806,C$)
DRAWFONT_BASIC PROC
	; G-VRAM状態を保存
	IN	A, (0E8h)
	LD	(BACKUP2_GVRAM_STATUS), A

	; G-VRAM有効
	AND	03Fh
	OR	080h
	OUT	(0E8h), A

	LD	A, (DE) ; X
	LD	(X), A
	INC	DE
	LD	A, (DE) ; Y
	LD	(Y), A
	INC	DE
	LD	A, (DE) ; C
	LD	(COLOR), A
	INC	DE
	LD	A, (DE) ; R
	LD	(NEWLINE), A
	INC	DE
	LD	A, (DE) ; ID
	LD	(ID), A
	INC	DE
	LD	A, (DE) ; I
	LD	(INDEX), A
	; 表示アドレス計算
	CALL	GET_DRAWADR
	; 文字イメージ取得
	LD	A, (INDEX)
	LD	C, A
	LD	A, (ID)
	CALL	GET_INDEX
	LD	C, A
	LD	A, (ID)
	CALL	GET_FONTIMAGE_ADR
	LD	BC, 16
	ADD	HL, BC
	; DE: バックアップする画面位置
	; A:  文字INDEX
	LD	A, (INDEX)
	CALL	IMAGE_TO_TEMP_ALL
	LD	A, (INDEX)
	CALL	TEMP_TO_BACKUP

	; DE = 表示位置
	; HL = 表示フォントマスクアドレス
	CALL	DRAW_FONTMASK
	; DE = 表示位置
	; HL = 表示フォントアドレス
	; B =  色
	LD	BC, -16
	ADD	HL, BC
	LD	A, (COLOR)
	LD	B, A
	CALL	DRAW_FONT

	; GVRAM状態を元に戻す
	LD	A, (BACKUP2_GVRAM_STATUS)
	OUT	(0E8h), A
	RET
ENDP

; 表示した文字を消す
; DE: 文字列のアドレス
; BC: 文字列の長さ
; C$=CHR$(ID)
; USR($B809,C$)
ERASE_BASIC PROC
	; G-VRAM状態を保存
	IN	A, (0E8h)
	LD	(BACKUP3_GVRAM_STATUS), A

	; G-VRAM有効
	AND	03Fh
	OR	080h
	OUT	(0E8h), A

	PUSH	BC
	PUSH	HL
	LD	A, (DE) ; ID
	LD	(ID), A
	CALL	GET_LENGTH
	LD	B, A
	LD	C, 0
LOOP:
	; 16x8サイズ3色のバックアップからテンポラリバッファに転送する
	LD	A, C
	LD	(INDEX), A
	CALL	BACKUP_TO_TEMP

	; 表示アドレス計算
	; DE = 描画アドレス
	CALL	GET_DRAWADR

	; テンポラリバッファの画像を表示する
	LD	A, 1
	OUT	(0F7h), A
	LD	HL, IMAGE_TEMP_TRANSFER
	PUSH	HL
	CALL	PUT16x8
	POP	HL
	LD	A, 2
	OUT	(0F7h), A
	LD	HL, IMAGE_TEMP_TRANSFER + 16
	PUSH	HL
	CALL	PUT16x8
	POP	HL
	LD	A, 3
	OUT	(0F7h), A
	LD	HL, IMAGE_TEMP_TRANSFER + 32
	PUSH	HL
	CALL	PUT16x8
	POP	HL

	INC	C
	DJNZ	LOOP
	POP	HL
	POP	BC

	; GVRAM状態を元に戻す
	LD	A, (BACKUP3_GVRAM_STATUS)
	OUT	(0E8h), A
	RET
ENDP

; マスク表示
; DE = 表示位置
; HL = 表示フォントマスクアドレス
DRAW_FONTMASK PROC
	LD	A, 1
	OUT	(0F7h), A
	CALL	PUT16x8_AND
	LD	A, 2
	OUT	(0F7h), A
	CALL	PUT16x8_AND
	LD	A, 3
	OUT	(0F7h), A
	JP	PUT16x8_AND
ENDP

; フォント表示
; DE = 表示位置
; HL = 表示フォントマスクアドレス
; B = 色 (0～7)
DRAW_FONT PROC
	BIT	0, B
	JR	Z, @f
	LD	A, 1
	OUT	(0F7h), A
	CALL	PUT16x8_OR
@@:
	BIT	1, B
	JR	Z, @f
	LD	A, 2
	OUT	(0F7h), A
	CALL	PUT16x8_OR
@@:
	BIT	2, B
	RET	Z
	LD	A, 3
	OUT	(0F7h), A
	JP	PUT16x8_OR
ENDP

; 文字列の長さを取得
; A: ID (1～)
; 破壊: なし
; Result: A: IDの文字列の長さ
GET_LENGTH PROC
	PUSH	DE
	PUSH	HL
	LD	HL, DATA
	INC	HL
	; HL = HL + (A - 1) * 2
	; ID 1始まりを0始まりに修正
	DEC	A
	; ID番目のアドレスを取得
	ADD	A, A
	ADD	A, L
	JR	NC, @f
	INC	H
@@:
	LD	L, A
	; HL = DATAの先頭アドレス
	; DE = DATAの先頭
	LD	E, (HL)
	INC	HL
	LD	D, (HL)
	; A = INDEXS_SIZE
	LD	A, (DE)
	POP	HL
	POP	DE
	RET
ENDP

; 文字を取得
; A: ID (1～)
; C: index
; 破壊: なし
; A: 文字
GET_INDEX PROC
	PUSH	DE
	PUSH	HL
	LD	HL, DATA
	INC	HL
	; HL = HL + (A - 1) * 2
	; ID 1始まりを0始まりに修正
	DEC	A
	; ID番目のアドレスを取得
	ADD	A, A
	ADD	A, L
	JR	NC, @f
	INC	H
@@:
	LD	L, A
	; HL = DATAの先頭アドレス
	; DE = DATAの先頭
	LD	E, (HL)
	INC	HL
	LD	D, (HL)
	; HL = INDEXS
	EX	DE, HL
	INC	HL
	; HL = HL + C
	LD	A, C
	ADD	A, L
	JR	NC, @f
	INC	H
@@:
	LD	L, A
	; A = 文字
	LD	A, (HL)
	POP	HL
	POP	DE
	RET
ENDP

; フォントイメージのアドレス取得
; A: ID (1～)
; C: 文字(0～)
; 破壊: A
; Result: HL: フォントイメージのアドレス
GET_FONTIMAGE_ADR PROC
	PUSH	BC
	PUSH	DE
	LD	HL, DATA
	INC	HL
	; HL = HL + (A - 1) * 2
	; ID 1始まりを0始まりに修正
	DEC	A
	; ID番目のアドレスを取得
	ADD	A, A
	ADD	A, L
	JR	NC, @f
	INC	H
@@:
	LD	L, A
	; HL = DATAの先頭アドレス
	; DE = DATAの先頭
	LD	E, (HL)
	INC	HL
	LD	D, (HL)
	; A = INDEXS_SIZE
	LD	A, (DE)
	; HL = INDEXSの先頭アドレス
	EX	DE, HL
	INC	HL
	; HL = HL + A
	ADD	A, L
	JR	NC, @f
	INC	H
@@:
	LD	L, A
	; B番目の文字のアドレスを求める
	LD	D, 0
	LD	E, C
	; HL = HL + B * 32
	EX	DE, HL
	ADD	HL, HL
	ADD	HL, HL
	ADD	HL, HL
	ADD	HL, HL
	ADD	HL, HL
	ADD	HL, DE
	POP	DE
	POP	BC
	RET
ENDP

; 表示アドレス
; X: 横位置 (0～78)
; Y: 縦位置 (0～24)
; INDEX: 文字位置
; NEWLINE: 改行文字数 (1～39)
; 破壊 A
; Result: DE = 描画アドレス
GET_DRAWADR PROC
	PUSH	BC
	PUSH	HL
	; 文字位置と改行位置を考慮する
	LD	A, (INDEX)
	ADD	A, A
	LD	B, A
	LD	A, (NEWLINE)
	ADD	A, A ; 改行幅を40桁にするため2倍にする
	LD	C, A
	; B / C = D ... E
	LD	A, B
	LD	B, 0
@@:
	SUB	C
	JR	C, @f
	DJNZ	@b
@@:
	ADD	A, C
	LD	E, A	; E: 余り
	LD	A, B
	NEG
	LD	D, A	; D: 商
	; HL = (Y + D) * 640(80 * 8ライン)
	LD	H, 0
	LD	A, (Y)
	ADD	A, D
	LD	L, A
	ADD	HL, HL ; *2
	ADD	HL, HL ; *4
	ADD	HL, HL ; *8
	ADD	HL, HL ; *16
	ADD	HL, HL ; *32
	ADD	HL, HL ; *64
	ADD	HL, HL ; *128
	LD	B, H
	LD	C, L
	ADD	HL, HL ; *256
	ADD	HL, HL ; *512
	ADD	HL, BC
	; HL = HL + (X + E)
	LD	A, (X)
	ADD	A, E
	ADD	A, L
	JR	NC, @f
	INC	H
@@:
	LD	L, A
	; HL = 0C000h + HL
	LD	BC, 0C000h
	ADD	HL, BC
	; DE = HL
	EX	DE, HL
	POP	HL
	POP	BC
	RET
ENDP

;16x8表示を転送する
;DE: POSITION
;HL: PATTERN
;破壊 A
;PUT16x8 PROC
;	PUSH	DE
;	PUSH	HL
;	LD	(PUT16x8_RESTORE_STACK + 1), SP
;	LD	SP, HL
;	EX	DE, HL
;	REPT 7
;	POP	DE
;	LD	(HL), E
;	INC	HL
;	LD	(HL), D
;	INC	HL
;	; HL=HL+78 30clock
;	LD	A, L	; 4
;	ADD	A, 78	; 7
;	LD	L, A	; 4
;	LD	A, H	; 4
;	ADC	A, 0	; 7
;	LD	H, A	; 4
;	ENDM
;	POP	DE
;	LD	(HL), E
;	INC	HL
;	LD	(HL), D
;PUT16x8_RESTORE_STACK:
;	LD	SP, 0000
;	POP	HL
;	POP	DE
;	RET
;ENDP

;16x8表示をAND転送する
;DE: POSITION
;HL: PATTERN
;破壊 A
PUT16x8_AND PROC
	PUSH	BC
	PUSH	DE
	PUSH	HL
	LD	(PUT16x8_AND_RESTORE_STACK + 1), SP
	LD	SP, HL
	EX	DE, HL
	LD	B, 7
@@:
	POP	DE
	LD	A, (HL)
	AND	E
	LD	(HL), A
	INC	HL
	LD	A, (HL)
	AND	D
	LD	(HL), A
	INC	HL
	; HL=HL+78 30clock
	LD	A, L	; 4
	ADD	A, 78	; 7
	LD	L, A	; 4
	LD	A, H	; 4
	ADC	A, 0	; 7
	LD	H, A	; 4
	DJNZ	@b
	POP	DE
	LD	A, (HL)
	AND	E
	LD	(HL), A
	INC	HL
	LD	A, (HL)
	AND	D
	LD	(HL), A
PUT16x8_AND_RESTORE_STACK:
	LD	SP, 0000
	POP	HL
	POP	DE
	POP	BC
	RET
ENDP

;16x8表示をAND転送する
;DE: POSITION
;HL: PATTERN
;破壊 A
PUT16x8_OR PROC
	PUSH	BC
	PUSH	DE
	PUSH	HL
	LD	(PUT16x8_OR_RESTORE_STACK + 1), SP
	LD	SP, HL
	EX	DE, HL
	LD	B, 7
@@:
	POP	DE
	LD	A, (HL)
	OR	E
	LD	(HL), A
	INC	HL
	LD	A, (HL)
	OR	D
	LD	(HL), A
	INC	HL
	; HL=HL+78 30clock
	LD	A, L	; 4
	ADD	A, 78	; 7
	LD	L, A	; 4
	LD	A, H	; 4
	ADC	A, 0	; 7
	LD	H, A	; 4
	DJNZ	@b
	POP	DE
	LD	A, (HL)
	OR	E
	LD	(HL), A
	INC	HL
	LD	A, (HL)
	OR	D
	LD	(HL), A
PUT16x8_OR_RESTORE_STACK:
	LD	SP, 0000
	POP	HL
	POP	DE
	POP	BC
	RET
ENDP

; 16x8サイズ8色を画面バックアップ用テンポラリバッファに転送する
; DE: バックアップする画面位置
; A:  文字INDEX
IMAGE_TO_TEMP_ALL PROC
	PUSH	HL
	LD	A, 1
	OUT	(0F7h), A
	LD	HL, IMAGE_TEMP_TRANSFER
	CALL	IMAGE_TO_TEMP_1PAGE
	LD	A, 2
	OUT	(0F7h), A
	LD	HL, IMAGE_TEMP_TRANSFER + 16
	CALL	IMAGE_TO_TEMP_1PAGE
	LD	A, 3
	OUT	(0F7h), A
	LD	HL, IMAGE_TEMP_TRANSFER + 32
	CALL	IMAGE_TO_TEMP_1PAGE
	POP	HL
	RET
ENDP

; 16x8サイズ1色を転送する
; DE: バックアップする画面位置
; HL: 転送先
; A:  文字INDEX
IMAGE_TO_TEMP_1PAGE PROC
	PUSH	BC
	PUSH	DE
	PUSH	HL
	EX	DE, HL
	LD	B, 7
@@:
	PUSH	BC
	LD	BC, 79
	LD	A, (HL)
	LD	(DE), A
	INC	DE
	INC	HL
	LD	A, (HL)
	LD	(DE), A
	INC	DE
	ADD	HL, BC
	POP	BC
	DJNZ	@b
	LD	A, (HL)
	LD	(DE), A
	INC	DE
	INC	HL
	LD	A, (HL)
	LD	(DE), A
	POP	HL
	POP	DE
	POP	BC
	RET
ENDP

; 16x8サイズ3色のテンポラリバッファをバックアップする
; A:  文字INDEX
TEMP_TO_BACKUP PROC
	PUSH	BC
	PUSH	DE
	PUSH	HL
	LD	B, A
	; GVRAMを無効にする
	IN	A, (0E8h)
	LD	(BACKUP4_GVRAM_STATUS), A
	AND	07Fh
	OUT	(0E8h), A
	; 転送先アドレスを計算する
	LD	H, 0
	LD	L, B
	ADD	HL, HL ; *2
	ADD	HL, HL ; *4
	ADD	HL, HL ; *8
	ADD	HL, HL ; *16
	LD	B, H
	LD	C, L
	ADD	HL, HL ; *32
	ADD	HL, BC ; *48
	LD	BC, 0C000h
	ADD	HL, BC
	EX	DE, HL ; DE: 0C000h + INDEX * 48
	LD	HL, IMAGE_TEMP_TRANSFER
	LD	BC, 48
	; LDIR (DE) ← (HL), BC Bytes
	LDIR
	; GVRAM状態を元に戻す
	LD	A, (BACKUP4_GVRAM_STATUS)
	OUT	(0E8h), A
	POP	HL
	POP	DE
	POP	BC
	RET
ENDP

; 16x8サイズ3色のバックアップからテンポラリバッファに転送する
; A:  文字INDEX
BACKUP_TO_TEMP PROC
	PUSH	BC
	PUSH	DE
	PUSH	HL
	LD	B, A
	; GVRAMを無効にする
	IN	A, (0E8h)
	LD	(BACKUP5_GVRAM_STATUS), A
	AND	07Fh
	OUT	(0E8h), A
	; 転送先アドレスを計算する
	LD	H, 0
	LD	L, B
	ADD	HL, HL ; *2
	ADD	HL, HL ; *4
	ADD	HL, HL ; *8
	ADD	HL, HL ; *16
	LD	B, H
	LD	C, L
	ADD	HL, HL ; *32
	ADD	HL, BC ; *48
	LD	BC, 0C000h
	ADD	HL, BC ; HL: 0C000h + INDEX * 48
	LD	DE, IMAGE_TEMP_TRANSFER
	LD	BC, 48
	; LDIR (DE) ← (HL), BC Bytes
	LDIR
	; GVRAM状態を元に戻す
	LD	A, (BACKUP5_GVRAM_STATUS)
	OUT	(0E8h), A
	POP	HL
	POP	DE
	POP	BC
	RET
ENDP

; ワークエリア

BACKUP1_GVRAM_STATUS:
	DB	0
BACKUP2_GVRAM_STATUS:
	DB	0
BACKUP3_GVRAM_STATUS:
	DB	0
BACKUP4_GVRAM_STATUS:
	DB	0
BACKUP5_GVRAM_STATUS:
	DB	0

; DRAWFONT_ALL_BASICバッファ
DRAW_ALL_WORK:
DRAW_ALL_X:
	DB	0
DRAW_ALL_Y:
	DB	0
DRAW_ALL_COLOR:
	DB	0
DRAW_ALL_NEWLINE:
	DB	0
DRAW_ALL_ID:
	DB	0
DRAW_ALL_INDEX:
	DB	0

; DRAWFONT_BASICバッファ
WORK:
X:
	DB	0
Y:
	DB	0
COLOR:
	DB	0
NEWLINE:
	DB	0
ID:
	DB	0
INDEX:
	DB	0

; 16x8画面バックアップ用テンポラリバッファ
IMAGE_TEMP_TRANSFER:
	DS	0

;	END	0B1h
