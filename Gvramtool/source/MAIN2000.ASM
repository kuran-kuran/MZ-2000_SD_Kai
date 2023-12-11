; LIMIT $B7FF
;PRG   B800h～
;STACK BF00h～BFFFh
;BUF   C000h～FE7Fh (8000/16000Bytes)
;GVRAM C000h～FE7Fh (16000Bytes)

; old info
; 0BD00h～0BDFDh stack
; 0BE00h GVRAMTORAM
; 0BE39h RAMTOGVRAM
; 0BE77h SaveC000_16000
	ORG 0B800h
	; BASIC CALL ADDRESS
	JP	GVRAMTORAM		; USR($B800) GVRAM(16KB) → RAM BUF
	JP	RAMTOGVRAM_WIDTHX2	; USR($B803) BUF320x200(8KB) → GVRAM(640x200) 横2倍
	JP	RAMTOGVRAM		; USR($B806) BUF640x200(16KB) → GVRAM(640x200) MZ-2000専用
	LD	HL, 0			; POKE$B80A,L:POKE$B80B,H:USR($B809) LZE展開
	JP	DECORD
	JP	SD_USR_LOAD		; USR($B80F, F$) MZ2000_SDバイナリロード

; GVRAMにあるデータをRAMに転送する
; GVRAM to RAM 0C000h～ 16000bytes
; GRAPH Iで指定されているページがRAMに転送されます
GVRAMTORAM:
	LD	(RESTORE_STACK1 + 1), SP
	LD	SP, STACK - 1

	; backup port 0E8h
	IN	A, (0E8h)
	LD	(BACKUP_PORT_0E8), A

	; 125word(250byte) x 64 = 16000byte
	LD	HL, 0C000h
	LD	B, 64
LOOP1:
	; 0C000h～をGVRAMに切り替え
	LD	A, 160
	OUT	(0E8h), A

	; G-VRAM to stack 125word (250byte)
	LD	C, 125
LOOP2:
	LD	D, (HL)
	INC	HL
	LD	E, (HL)
	INC	HL
	PUSH	DE
	DEC	C
	JR	NZ, LOOP2

	; 0C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A

	; Stack to RAM 125word (250byte)
	LD	C, 125
LOOP3:
	POP	DE
	DEC	HL
	LD	(HL), E
	DEC	HL
	LD	(HL), D
	DEC	C
	JR	NZ, LOOP3

	; HLを250byte進める
	LD	DE, 250
	ADD	HL, DE

	DEC	B
	JR	NZ, LOOP1

	; restore SP
RESTORE_STACK1:
	LD	SP, 0000
	RET

; RAMにあるデータをGVRAMに転送する
; RAM to GVRAM 0C000h～ 16000bytes
; RAMからGRAPH Iで指定されているページに転送されます
RAMTOGVRAM:
	LD	(RESTORE_STACK2 + 1), SP
	LD	SP, STACK - 1

	; backup port 0E8h
	IN	A, (0E8h)
	LD	(BACKUP_PORT_0E8), A

	; 125word(250byte) x 64 = 16000byte
	LD	HL, 0C000h
	LD	B, 64
LOOP4:
	; 0C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A

	; G-VRAM to stack 125word (250byte)
	LD	C, 125
LOOP5:
	LD	D, (HL)
	INC	HL
	LD	E, (HL)
	INC	HL
	PUSH	DE
	DEC	C
	JR	NZ, LOOP5

	; 0C000h～をGVRAMに切り替え
	LD	A, 160
	OUT	(0E8h), A

	; Stack to RAM (250byte)
	LD	C, 125
LOOP6:
	POP	DE
	DEC	HL
	LD	(HL), E
	DEC	HL
	LD	(HL), D
	DEC	C
	JR	NZ, LOOP6

	; HLを250byte進める
	LD	DE, 250
	ADD	HL, DE

	DEC	B
	JR	NZ, LOOP4

	; 0C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A

	; restore SP
RESTORE_STACK2:
	LD	SP, 0000
	RET

; RAMにあるデータを幅2倍にしてGVRAMに転送する
; RAM to GVRAM 0C000h～ 8000bytes
; RAMからGRAPH Iで指定されているページに転送されます
RAMTOGVRAM_WIDTHX2:
	LD	(RESTORE_STACK3 + 1), SP
	LD	SP, STACK - 1

	; backup port 0E8h
	IN	A, (0E8h)
	LD	(BACKUP_PORT_0E8), A

	; 100word(200byte) x 40 = 8000byte
	LD	HL, 0C000h ; データ位置
	LD	IY, 0C000h ; 描画位置
	LD	B, 40
RAMTOGVRAM_WIDTHX2_LOOP4:
	; 0C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A

	; G-VRAM to stack 100word (200byte)
	LD	C, 100
RAMTOGVRAM_WIDTHX2_LOOP5:
	LD	D, (HL)
	INC	HL
	LD	E, (HL)
	INC	HL
	PUSH	DE
	DEC	C
	JR	NZ, RAMTOGVRAM_WIDTHX2_LOOP5

	; 0C000h～をGVRAMに切り替え
	LD	A, 160
	OUT	(0E8h), A

	LD	DE, 400
	ADD	IY, DE

	; Stack to RAM (200byte)
	LD	C, 100
RAMTOGVRAM_WIDTHX2_LOOP6:
	POP	DE
	DEC	IY
	DEC	IY
	LD	A, E
	CALL	PUT_WIDTHX2
;	LD	(HL), E
	DEC	IY
	DEC	IY
	LD	A, D
	CALL	PUT_WIDTHX2
;	LD	(HL), D
	DEC	C
	JR	NZ, RAMTOGVRAM_WIDTHX2_LOOP6

	; データ取得位置を合わせるためHLを200byte進める
;	LD	DE, 200
;	ADD	HL, DE
	; 描画位置を合わせるためIYを200byte進める
	LD	DE, 400
	ADD	IY, DE

	DEC	B
	JR	NZ, RAMTOGVRAM_WIDTHX2_LOOP4

	; 0C000h～RAMに戻す
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A

	; restore SP
RESTORE_STACK3:
	LD	SP, 0000
	RET

; IYの場所にAを16ビット拡張してセットする
PUT_WIDTHX2:
	LD	IX, WIDTHX2TABLE
	PUSH	DE
	LD	D, 0
	LD	E, A	; DE=A
	SLA	E
	RL	D	; DE=DE*2
	ADD	IX, DE	; IX=WIDTHX2TABLE+DE*2
	POP	DE
	LD	A, (IX)
	LD	(IY), A
	INC	IY
	LD	A, (IX + 1)
	LD	(IY), A
	DEC	IY
	RET

; デコードする
; HL: lzeデータ + 4
DECORD:
	LD	DE, 0DF00h
	CALL	DECODE_LZE
	RET

BACKUP_PORT_0E8:
	DB	0

WIDTHX2TABLE:
        DB 000h, 000h, 003h, 000h, 00Ch, 000h, 00Fh, 000h, 030h, 000h, 033h, 000h, 03Ch, 000h, 03Fh, 000h
        DB 0C0h, 000h, 0C3h, 000h, 0CCh, 000h, 0CFh, 000h, 0F0h, 000h, 0F3h, 000h, 0FCh, 000h, 0FFh, 000h
        DB 000h, 003h, 003h, 003h, 00Ch, 003h, 00Fh, 003h, 030h, 003h, 033h, 003h, 03Ch, 003h, 03Fh, 003h
        DB 0C0h, 003h, 0C3h, 003h, 0CCh, 003h, 0CFh, 003h, 0F0h, 003h, 0F3h, 003h, 0FCh, 003h, 0FFh, 003h
        DB 000h, 00Ch, 003h, 00Ch, 00Ch, 00Ch, 00Fh, 00Ch, 030h, 00Ch, 033h, 00Ch, 03Ch, 00Ch, 03Fh, 00Ch
        DB 0C0h, 00Ch, 0C3h, 00Ch, 0CCh, 00Ch, 0CFh, 00Ch, 0F0h, 00Ch, 0F3h, 00Ch, 0FCh, 00Ch, 0FFh, 00Ch
        DB 000h, 00Fh, 003h, 00Fh, 00Ch, 00Fh, 00Fh, 00Fh, 030h, 00Fh, 033h, 00Fh, 03Ch, 00Fh, 03Fh, 00Fh
        DB 0C0h, 00Fh, 0C3h, 00Fh, 0CCh, 00Fh, 0CFh, 00Fh, 0F0h, 00Fh, 0F3h, 00Fh, 0FCh, 00Fh, 0FFh, 00Fh
        DB 000h, 030h, 003h, 030h, 00Ch, 030h, 00Fh, 030h, 030h, 030h, 033h, 030h, 03Ch, 030h, 03Fh, 030h
        DB 0C0h, 030h, 0C3h, 030h, 0CCh, 030h, 0CFh, 030h, 0F0h, 030h, 0F3h, 030h, 0FCh, 030h, 0FFh, 030h
        DB 000h, 033h, 003h, 033h, 00Ch, 033h, 00Fh, 033h, 030h, 033h, 033h, 033h, 03Ch, 033h, 03Fh, 033h
        DB 0C0h, 033h, 0C3h, 033h, 0CCh, 033h, 0CFh, 033h, 0F0h, 033h, 0F3h, 033h, 0FCh, 033h, 0FFh, 033h
        DB 000h, 03Ch, 003h, 03Ch, 00Ch, 03Ch, 00Fh, 03Ch, 030h, 03Ch, 033h, 03Ch, 03Ch, 03Ch, 03Fh, 03Ch
        DB 0C0h, 03Ch, 0C3h, 03Ch, 0CCh, 03Ch, 0CFh, 03Ch, 0F0h, 03Ch, 0F3h, 03Ch, 0FCh, 03Ch, 0FFh, 03Ch
        DB 000h, 03Fh, 003h, 03Fh, 00Ch, 03Fh, 00Fh, 03Fh, 030h, 03Fh, 033h, 03Fh, 03Ch, 03Fh, 03Fh, 03Fh
        DB 0C0h, 03Fh, 0C3h, 03Fh, 0CCh, 03Fh, 0CFh, 03Fh, 0F0h, 03Fh, 0F3h, 03Fh, 0FCh, 03Fh, 0FFh, 03Fh
        DB 000h, 0C0h, 003h, 0C0h, 00Ch, 0C0h, 00Fh, 0C0h, 030h, 0C0h, 033h, 0C0h, 03Ch, 0C0h, 03Fh, 0C0h
        DB 0C0h, 0C0h, 0C3h, 0C0h, 0CCh, 0C0h, 0CFh, 0C0h, 0F0h, 0C0h, 0F3h, 0C0h, 0FCh, 0C0h, 0FFh, 0C0h
        DB 000h, 0C3h, 003h, 0C3h, 00Ch, 0C3h, 00Fh, 0C3h, 030h, 0C3h, 033h, 0C3h, 03Ch, 0C3h, 03Fh, 0C3h
        DB 0C0h, 0C3h, 0C3h, 0C3h, 0CCh, 0C3h, 0CFh, 0C3h, 0F0h, 0C3h, 0F3h, 0C3h, 0FCh, 0C3h, 0FFh, 0C3h
        DB 000h, 0CCh, 003h, 0CCh, 00Ch, 0CCh, 00Fh, 0CCh, 030h, 0CCh, 033h, 0CCh, 03Ch, 0CCh, 03Fh, 0CCh
        DB 0C0h, 0CCh, 0C3h, 0CCh, 0CCh, 0CCh, 0CFh, 0CCh, 0F0h, 0CCh, 0F3h, 0CCh, 0FCh, 0CCh, 0FFh, 0CCh
        DB 000h, 0CFh, 003h, 0CFh, 00Ch, 0CFh, 00Fh, 0CFh, 030h, 0CFh, 033h, 0CFh, 03Ch, 0CFh, 03Fh, 0CFh
        DB 0C0h, 0CFh, 0C3h, 0CFh, 0CCh, 0CFh, 0CFh, 0CFh, 0F0h, 0CFh, 0F3h, 0CFh, 0FCh, 0CFh, 0FFh, 0CFh
        DB 000h, 0F0h, 003h, 0F0h, 00Ch, 0F0h, 00Fh, 0F0h, 030h, 0F0h, 033h, 0F0h, 03Ch, 0F0h, 03Fh, 0F0h
        DB 0C0h, 0F0h, 0C3h, 0F0h, 0CCh, 0F0h, 0CFh, 0F0h, 0F0h, 0F0h, 0F3h, 0F0h, 0FCh, 0F0h, 0FFh, 0F0h
        DB 000h, 0F3h, 003h, 0F3h, 00Ch, 0F3h, 00Fh, 0F3h, 030h, 0F3h, 033h, 0F3h, 03Ch, 0F3h, 03Fh, 0F3h
        DB 0C0h, 0F3h, 0C3h, 0F3h, 0CCh, 0F3h, 0CFh, 0F3h, 0F0h, 0F3h, 0F3h, 0F3h, 0FCh, 0F3h, 0FFh, 0F3h
        DB 000h, 0FCh, 003h, 0FCh, 00Ch, 0FCh, 00Fh, 0FCh, 030h, 0FCh, 033h, 0FCh, 03Ch, 0FCh, 03Fh, 0FCh
        DB 0C0h, 0FCh, 0C3h, 0FCh, 0CCh, 0FCh, 0CFh, 0FCh, 0F0h, 0FCh, 0F3h, 0FCh, 0FCh, 0FCh, 0FFh, 0FCh
        DB 000h, 0FFh, 003h, 0FFh, 00Ch, 0FFh, 00Fh, 0FFh, 030h, 0FFh, 033h, 0FFh, 03Ch, 0FFh, 03Fh, 0FFh
        DB 0C0h, 0FFh, 0C3h, 0FFh, 0CCh, 0FFh, 0CFh, 0FFh, 0F0h, 0FFh, 0F3h, 0FFh, 0FCh, 0FFh, 0FFh, 0FFh

INCLUDE "lzdec.mac"
INCLUDE "MZ2000SD.ASM"

	ORG	0C000h
STACK:

	END	000B1h
