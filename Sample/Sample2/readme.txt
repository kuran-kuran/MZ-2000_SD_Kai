MZ2000_SD�T���v���v���O����2

���͂��߂�
SD�J�[�h����320x200�̉摜��ǂݍ���ŉ�ʂɕ\������T���v���v���O�����ł�
MZ-80B/2000/MZ-2200/MZ-2500��2000�A80B���[�h�œ��삵�܂��B
���̃v���O���������s����ɂ�MZ2000_SD��Arduino�̃v���O�����������ď���������K�v������܂��B

���t�@�C���ꗗ
anime80b   MZ-80B�p�摜�\���T���v���ꎮ
anime2000  MZ-2000/2200�p�摜�\���T���v���ꎮ
readme.txt ���̃t�@�C��

���N�����@
MZ-80B�̏ꍇ
(1) anime80b�t�H���_���̃t�@�C�����}�C�N��SD�J�[�h�ɃR�s�[���܂�
(2) MZ-80B���N������MZ2000_SD����MZ2000_SD�ɑΉ�����SB-5520���N�����܂�
(3) LOAD���߂����s����ANIME80B�����[�h���܂�
(4) RUN����ƃt�@�C�����𕷂��Ă���̂�GVRAMTOOL80B�Ƒł����݂܂�

MZ-2000/2200�̏ꍇ
(1) anime2000�t�H���_���̃t�@�C�����}�C�N��SD�J�[�h�ɃR�s�[���܂�
(2) MZ-2000���N������MZ2000_SD����MZ2000_SD�ɑΉ�����MZ-1Z001���N�����܂�
(3) LOAD���߂����s����ANIME�����[�h���܂�
(4) RUN����ƃt�@�C�����𕷂��Ă���̂�GVRAMTOOL2000�Ƒł����݂܂�

��MZ-80B �������}�b�v
GVRAM 6000h�`7F3Fh (8000Bytes)
RPG   D800h�`
STACK DE00h�`DEFFh
BUF   DF00h�`FE3Fh (8000Bytes)

��MZ-80B�}�V����Ăяo��
LIMIT$D7FF:LOAD "GVRAMTOOL80B"     �}�V����ǂݍ��݁BMZ2000_SD�ł�DOSFILE�𕷂���邽��GVRAMTOOL80B�Ɠ��͂��Ă�������
USR($D800)                         GVRAM����RAM�ɃR�s�[���� GVRAM(8000Bytes) �� BUF
USR($D803)                         BUF�ɂ���320x200(8000bytes)��GVRAM�ɃR�s�[����
USR($D806)                         �Ȃɂ����Ȃ��B����
POKE$D80A,L:POKE$D80B,H:USR($D809) LZE��BUF�ɓW�J����BL��H��LZE��ǂݍ��񂾃A�h���X+4���w�肷��
F$="FILENAME":USR($D80F,F$)        MZ2000_SD����o�C�i���t�@�C�������[�h����
USR($D812,F$)                      MZ2000_SD�A���t�@�C���I�[�v��
USR($D815)                         MZ2000_SD�A���t�@�C�����[�h
USR($D818)                         MZ2000_SD�A���t�@�C���X�L�b�v
USR($D81B,F$)                      MZ2000_SD�A���t�@�C������
USR($D81E)                         MZ2000_SD�A���t�@�C���擪�փV�[�N
USR($D821)                         MZ2000_SD�A���t�@�C���N���[�Y

��MZ-2000/2200 �������}�b�v
PRG   B800h�`
STACK BF00h�`BFFFh
BUF   C000h�`FE7Fh (8000/16000Bytes)
GVRAM C000h�`FE7Fh (16000Bytes)

��MZ-2000/2200�}�V����Ăяo��
LIMIT$B7FF:LOAD "GVRAMTOOL2000"    �}�V����ǂݍ��݁BMZ2000_SD�ł�DOSFILE�𕷂���邽��GVRAMTOOL2000�Ɠ��͂��Ă�������
USR($B800)                         GVRAM����RAM�ɃR�s�[���� GVRAM(16000Bytes) �� BUF
USR($B803)                         BUF�ɂ���320x200(8000bytes)��GVRAM�ɉ�2�{�Ɉ����L�΂��R�s�[����
USR($B806)                         BUF�ɂ���640x200(16000bytes)��GVRAM�ɃR�s�[����
POKE$B80A,L:POKE$B80B,H:USR($B809) LZE��BUF�ɓW�J����BL��H��LZE��ǂݍ��񂾃A�h���X+4���w�肷��
F$="FILENAME":USR($B80F,F$)        MZ2000_SD����o�C�i���t�@�C�������[�h����
USR($B812,F$)                      MZ2000_SD�A���t�@�C���I�[�v��
USR($B815)                         MZ2000_SD�A���t�@�C�����[�h
USR($B818)                         MZ2000_SD�A���t�@�C���X�L�b�v
USR($B81B,F$)                      MZ2000_SD�A���t�@�C������
USR($B81E)                         MZ2000_SD�A���t�@�C���擪�փV�[�N
USR($B821)                         MZ2000_SD�A���t�@�C���N���[�Y
