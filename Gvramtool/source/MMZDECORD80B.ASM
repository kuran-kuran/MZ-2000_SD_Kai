DRAW_MMZ:
DRAW_MMZ1:
	; データロード
	CALL	SD_READ_CONCAT_1BLOCK
	; データ展開
	LD	HL, 0DF00h + 4
	LD	DE, 0B800h
	CALL	DECODE_LZE
	; MMZデコード
	LD	HL, 0B800h
	CALL	DECORD_MMZ
	CP	0Ah
	JR	Z, DRAW_MMZ1
	; 0Ah以外 (0Bh終了)
	RET

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
	OR	0C0h
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
	CALL	PUT8x8_Color
	DEC	C
	JR	NZ, DECORD_MMZ_3
	JR	DECORD_MMZ_1
COMMAND_PAUSE:
COMMAND_END:
	; C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A
	RET

;8x8を指定プレーン分表示する
;B : プレーン数
;DE: POSITION(2の倍数)
;HL: PATTERN
;破壊 A
;     DE: 次の座標
;     HL: 次のデータ
PUT8x8_Color:
	BIT	0, B
	JR	Z, @F
	LD	A, 6 ; GRAPH I1,O12
	OUT	(0F4h), A
	CALL	PUT8x8
@@:
	BIT	1, B
	JR	Z, @F
	LD	A, 7 ; GRAPH I2,O12
	OUT	(0F4h), A
	CALL	PUT8x8
@@:
	; DE=DE+1
	INC	DE
	RET

;8x8表示を転送する
;DE: POSITION(2の倍数)
;HL: PATTERN
;破壊 A
;     HL=HL+8
PUT8x8:
	PUSH	BC
	PUSH	DE
	PUSH	HL
	LD	BC, 40
	LD	(PUT8x8_RESTORE_STACK + 1), SP
	LD	SP, HL
	EX	DE, HL
	REPT 3
	POP	DE
	LD	(HL), E
	ADD	HL, BC
	LD	(HL), D
	ADD	HL, BC
	ENDM
	POP	DE
	LD	(HL), E
	ADD	HL, BC
	LD	(HL), D
PUT8x8_RESTORE_STACK:
	LD	SP, 0000
	POP	HL
	POP	DE
	LD	BC, 8
	ADD	HL, BC
	POP	BC
	RET
