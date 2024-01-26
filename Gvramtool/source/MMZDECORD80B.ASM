DRAW_MMZ:
DRAW_MMZ1:
	; �f�[�^���[�h
	CALL	SD_READ_CONCAT_1BLOCK
	; �f�[�^�W�J
	LD	HL, 0DF00h + 4
	LD	DE, 0B800h
	CALL	DECODE_LZE
	; MMZ�f�R�[�h
	LD	HL, 0B800h
	CALL	DECORD_MMZ
	CP	0Ah
	JR	Z, DRAW_MMZ1
	; 0Ah�ȊO (0Bh�I��)
	RET

;Mmz���f�R�[�h���ĉ�ʂɕ`�悷��
;HL: Mmz�f�[�^
;Result
;A: 0Ah: �u���b�N�`��I��
;   0Bh: 1��ʕ`��I��
DECORD_MMZ:
	; backup port 0E8h
	IN	A, (0E8h)
	LD	(BACKUP_PORT_0E8), A
	; C000h�`VRAM�o��
	OR	0C0h
	OUT	(0E8h), A
	; �R�}���h�擾
DECORD_MMZ_1:
	LD	A, (HL)
	CP	0Ah
	JP	C, COMMAND_PLANE ; 0Ah���� (1�`7�͂�����,0,8,9�͗��Ȃ��͂�)
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
	; B=�`��v���[���t���O
	; C�`���
	LD	B, (HL)
	INC	HL
	LD	C, (HL)
	INC	HL
	; �L�����N�^�\���J�n C�񃋁[�v(�ő�40��)
DECORD_MMZ_3:
	; B=�`��v���[���t���O
	; HL=PATTERN
	; DE=POSITION
	CALL	PUT8x8_Color
	DEC	C
	JR	NZ, DECORD_MMZ_3
	JR	DECORD_MMZ_1
COMMAND_PAUSE:
COMMAND_END:
	; C000h�`RAM�ɖ߂�
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A
	RET

;8x8���w��v���[�����\������
;B : �v���[����
;DE: POSITION(2�̔{��)
;HL: PATTERN
;�j�� A
;     DE: ���̍��W
;     HL: ���̃f�[�^
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

;8x8�\����]������
;DE: POSITION(2�̔{��)
;HL: PATTERN
;�j�� A
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