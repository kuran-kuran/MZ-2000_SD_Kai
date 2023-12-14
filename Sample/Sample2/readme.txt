MZ2000_SDサンプルプログラム2

■はじめに
SDカードから320x200の画像を読み込んで画面に表示するサンプルプログラムです
MZ-80B/2000/MZ-2200/MZ-2500の2000、80Bモードで動作します。
このプログラムを実行するにはMZ2000_SDのArduinoのプログラム改造して書き換える必要があります。

■ファイル一覧
anime80b   MZ-80B用画像表示サンプル一式
anime2000  MZ-2000/2200用画像表示サンプル一式
readme.txt このファイル

■起動方法
MZ-80Bの場合
(1) anime80bフォルダ内のファイルをマイクロSDカードにコピーします
(2) MZ-80Bを起動してMZ2000_SDからMZ2000_SDに対応したSB-5520を起動します
(3) LOAD命令を実行してANIME80Bをロードします
(4) RUNするとファイル名を聞いてくるのでGVRAMTOOL80Bと打ち込みます

MZ-2000/2200の場合
(1) anime2000フォルダ内のファイルをマイクロSDカードにコピーします
(2) MZ-2000を起動してMZ2000_SDからMZ2000_SDに対応したMZ-1Z001を起動します
(3) LOAD命令を実行してANIMEをロードします
(4) RUNするとファイル名を聞いてくるのでGVRAMTOOL2000と打ち込みます

■MZ-80B メモリマップ
GVRAM 6000h～7F3Fh (8000Bytes)
RPG   D800h～
STACK DE00h～DEFFh
BUF   DF00h～FE3Fh (8000Bytes)

■MZ-80Bマシン語呼び出し
LIMIT$D7FF:LOAD "GVRAMTOOL80B"     マシン語読み込み。MZ2000_SDではDOSFILEを聞かれるためGVRAMTOOL80Bと入力してください
USR($D800)                         GVRAMからRAMにコピーする GVRAM(8000Bytes) → BUF
USR($D803)                         BUFにある320x200(8000bytes)をGVRAMにコピーする
USR($D806)                         なにもしない。欠番
POKE$D80A,L:POKE$D80B,H:USR($D809) LZEをBUFに展開する。LとHはLZEを読み込んだアドレス+4を指定する
F$="FILENAME":USR($D80F,F$)        MZ2000_SDからバイナリファイルをロードする
USR($D812,F$)                      MZ2000_SD連結ファイルオープン
USR($D815)                         MZ2000_SD連結ファイルロード
USR($D818)                         MZ2000_SD連結ファイルスキップ
USR($D81B,F$)                      MZ2000_SD連結ファイル検索
USR($D81E)                         MZ2000_SD連結ファイル先頭へシーク
USR($D821)                         MZ2000_SD連結ファイルクローズ

■MZ-2000/2200 メモリマップ
PRG   B800h～
STACK BF00h～BFFFh
BUF   C000h～FE7Fh (8000/16000Bytes)
GVRAM C000h～FE7Fh (16000Bytes)

■MZ-2000/2200マシン語呼び出し
LIMIT$B7FF:LOAD "GVRAMTOOL2000"    マシン語読み込み。MZ2000_SDではDOSFILEを聞かれるためGVRAMTOOL2000と入力してください
USR($B800)                         GVRAMからRAMにコピーする GVRAM(16000Bytes) → BUF
USR($B803)                         BUFにある320x200(8000bytes)をGVRAMに横2倍に引き伸ばしコピーする
USR($B806)                         BUFにある640x200(16000bytes)をGVRAMにコピーする
POKE$B80A,L:POKE$B80B,H:USR($B809) LZEをBUFに展開する。LとHはLZEを読み込んだアドレス+4を指定する
F$="FILENAME":USR($B80F,F$)        MZ2000_SDからバイナリファイルをロードする
USR($B812,F$)                      MZ2000_SD連結ファイルオープン
USR($B815)                         MZ2000_SD連結ファイルロード
USR($B818)                         MZ2000_SD連結ファイルスキップ
USR($B81B,F$)                      MZ2000_SD連結ファイル検索
USR($B81E)                         MZ2000_SD連結ファイル先頭へシーク
USR($B821)                         MZ2000_SD連結ファイルクローズ
