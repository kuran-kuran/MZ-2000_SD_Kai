DRAW_MMZ:
DRAW_MMZ1:
	; データロード
	CALL	SD_READ_CONCAT_1BLOCK
	; データ展開
DRAW_MMZ2:
	LD	HL, 0C000h + 4
	LD	DE, 09800h
	CALL	DECODE_LZE
	; MMZデコード
	LD	HL, 09800h
	CALL	DECORD_MMZ
	CP	0Ah
	JR	Z, DRAW_MMZ1
	; 0Ah以外 (0Bh終了)
	RET

; 指定されたMMZファイルを読み込み画像1画面分表示
DRAW_MMZ_FILE:
	CALL	SD_USR_LOAD
	JR	DRAW_MMZ2

;Mmzをデコードして画面に描画する
;HL: Mmzデータ
;Result
;A: 0Ah: ブロック描画終了
;   0Bh: 1画面描画終了
DECORD_MMZ:
	; backup port 0E8h
	IN	A, (0E8h)
	LD	(BACKUP_PORT_0E8), A
	; C000h～VRAM出現
	AND	030h
	OR	080h
	OUT	(0E8h), A
	; コマンド取得
DECORD_MMZ_1:
	LD	A, (HL)
	CP	0Ah
	JP	C, COMMAND_PLANE ; 0Ah未満 (1～7はこちら,0,8,9は来ないはず)
	JR	Z, COMMAND_PAUSE ; 0Ah
	CP	0Bh
	JR	Z, COMMAND_END   ; 0Bh
COMMAND_ADDRESS:
	; DE=POSITION
	LD	D, A
	INC	HL
	LD	E, (HL)
	INC	HL
	JR	DECORD_MMZ_1
COMMAND_PLANE:
	; B=描画プレーンフラグ
	; C描画個数
	LD	B, (HL)
	INC	HL
	LD	C, (HL)
	INC	HL
	; キャラクタ表示開始 C回ループ(最大40回)
DECORD_MMZ_3:
	; B=描画プレーンフラグ
	; HL=PATTERN
	; DE=POSITION
	CALL	PUT16x8_Color
	DEC	C
	JR	NZ, DECORD_MMZ_3
	JR	DECORD_MMZ_1
COMMAND_PAUSE:
COMMAND_END:
	; C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A
	RET

;16x8を指定プレーン分表示する
;B : プレーン数
;DE: POSITION(2の倍数)
;HL: PATTERN
;破壊 A
;     DE: 次の座標
;     HL: 次のデータ
PUT16x8_Color:
	BIT	0, B
	JR	Z, @F
	LD	A, 1
	OUT	(0F7h), A
	CALL	PUT16x8
@@:
	BIT	1, B
	JR	Z, @F
	LD	A, 2
	OUT	(0F7h), A
	CALL	PUT16x8
@@:
	BIT	2, B
	JR	Z, @F
	LD	A, 3
	OUT	(0F7h), A
	CALL	PUT16x8
@@:
	; DE=DE+2
	INC	DE
	INC	DE
	RET

;16x8表示を転送する
;DE: POSITION(2の倍数)
;HL: PATTERN
;破壊 A
;     HL=HL+16
PUT16x8:
	PUSH	DE
	PUSH	HL
	LD	(PUT16x8_RESTORE_STACK + 1), SP
	LD	SP, HL
	EX	DE, HL
	REPT 7
	POP	DE
	LD	(HL), E
	INC	L
	LD	(HL), D
	INC	HL
	; HL=HL+78 30clock
	LD	A, L	; 4
	ADD	A, 78	; 7
	LD	L, A	; 4
	LD	A, H	; 4
	ADC	A, 0	; 7
	LD	H, A	; 4
	ENDM
	POP	DE
	LD	(HL), E
	INC	L
	LD	(HL), D
PUT16x8_RESTORE_STACK:
	LD	SP, 0000
	POP	HL
	POP	DE
	; HL=HL+16 30clock
	LD	A, L	; 4
	ADD	A, 16	; 7
	LD	L, A	; 4
	LD	A, H	; 4
	ADC	A, 0	; 7
	LD	H, A	; 4
	RET
