DRAW_MMZ:
DRAW_MMZ1:
	; �f�[�^���[�h
	CALL	SD_READ_CONCAT_1BLOCK
	; �f�[�^�W�J
	LD	HL, 0C000h + 4
	LD	DE, 09800h
	CALL	DECODE_LZE
	; MMZ�f�R�[�h
	LD	HL, 09800h
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
	AND	030h
	OR	080h
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
	CALL	PUT16x8_Color
	DEC	C
	JR	NZ, DECORD_MMZ_3
	JR	DECORD_MMZ_1
COMMAND_PAUSE:
COMMAND_END:
	; C000h�`RAM�ɖ߂�
	LD	A, (BACKUP_PORT_0E8)
	OUT	(0E8h), A
	RET

;16x8���w��v���[�����\������
;B : �v���[����
;DE: POSITION(2�̔{��)
;HL: PATTERN
;�j�� A
;     DE: ���̍��W
;     HL: ���̃f�[�^
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

;16x8�\����]������
;DE: POSITION(2�̔{��)
;HL: PATTERN
;�j�� A
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