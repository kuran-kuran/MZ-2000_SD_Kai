// 2022. 1.24 ファイルネームの後ろの20h詰めを0dhに修正するための処理をArduino側からMZ-80K側に修正
//           比較演算子の記述を見直し
// 2022. 1.25 各コマンド受信時のdelay()を廃止
// 2022. 1.26 FDコマンドでロード可能なファイル種類コードは0x01のみとしていた制限を撤廃
// 2022. 1.29 FDPコマンドのbug修正
// 2022. 1.30 FDLコマンド仕様変更 FDL A～Zの場合、ファイル名先頭一文字を比較して一致したものだけを出力
// 2022. 1.31 FDLコマンド仕様変更 FDL xの場合、ファイル名先頭一文字を比較して一致したものだけを出力
//            Bキーで前の20件を表示
// 2022. 2. 2 DOSファイル名がアルファベット小文字でもFDL xで検索できるように修正
// 2022. 2. 4 MZ-1200対策　初期化時にdelay(1000)を追加
// 2022. 2. 8 FDLコマンド仕様変更 FDL xの場合、ファイル名先頭1文字～32文字までに拡張
// 2023. 6.19 MZ-2000_SDの起動方式追加によりBOOT LOADER読み込みを追加。MZ-80K_SDには影響なし。
// 2024. 3. 4 sd-card再挿入時の初期化処理を追加
//
// 以下kuran_kuranが追加
// 2023.12.12 連結ファイル対応
// 2024.01.21 連結ファイルオープン中の通常ファイルアクセスに対応
// 2024.05.08 MIDIコマンド送信に対応
// 2025.04.26 D88ファイルに対応
//
#include "SdFat.h"
#include <SPI.h>
SdFat SD;
unsigned long m_lop=128;
char m_name[40];
byte s_data[260];
char f_name[40];
char c_name[40];
char new_name[40];
// 連結ファイル
char concatName[40];
char isConcatState = 0; // 0:未使用, 1:オープンしている
File concatFile;
unsigned long concatPos = 0;
unsigned long concatSize = 0;
// D88
char d88Name[40];
char isD88State = 0; // 0:未使用, 1:オープンしている
bool reverse = false;
File d88File;
int seekInfoPointer = 0;
int seekDataPointer = 0;
int seekDataOffset = 0;
char c = 0;
char h = 0;
char r = 0;
char n = 0;
short numberOfSector = 0;
short sizeOfData = 0;
short sectorsPerTrack = 0;

#define CABLESELECTPIN  (10)
#define CHKPIN          (15)
#define PB0PIN          (2)
#define PB1PIN          (3)
#define PB2PIN          (4)
#define PB3PIN          (5)
#define PB4PIN          (6)
#define PB5PIN          (7)
#define PB6PIN          (8)
#define PB7PIN          (9)
#define FLGPIN          (14)
#define PA0PIN          (16)
#define PA1PIN          (17)
#define PA2PIN          (18)
#define PA3PIN          (19)
// ファイル名は、ロングファイルネーム形式対応
boolean eflg;

void sdinit(void){
  // SD初期化
  if( !SD.begin(CABLESELECTPIN,8) )
  {
////    Serial.println("Failed : SD.begin");
    eflg = true;
  } else {
////    Serial.println("OK : SD.begin");
    eflg = false;
  }
////    Serial.println("START");
}

void setup(){
  Serial.begin(31250); // MIDI

////  Serial.begin(9600);
// CS=pin10
// pin10 output

  pinMode(CABLESELECTPIN,OUTPUT);
  pinMode( CHKPIN,INPUT);  //CHK
  pinMode( PB0PIN,OUTPUT); //送信データ
  pinMode( PB1PIN,OUTPUT); //送信データ
  pinMode( PB2PIN,OUTPUT); //送信データ
  pinMode( PB3PIN,OUTPUT); //送信データ
  pinMode( PB4PIN,OUTPUT); //送信データ
  pinMode( PB5PIN,OUTPUT); //送信データ
  pinMode( PB6PIN,OUTPUT); //送信データ
  pinMode( PB7PIN,OUTPUT); //送信データ
  pinMode( FLGPIN,OUTPUT); //FLG

  pinMode( PA0PIN,INPUT_PULLUP); //受信データ
  pinMode( PA1PIN,INPUT_PULLUP); //受信データ
  pinMode( PA2PIN,INPUT_PULLUP); //受信データ
  pinMode( PA3PIN,INPUT_PULLUP); //受信データ

  digitalWrite(PB0PIN,LOW);
  digitalWrite(PB1PIN,LOW);
  digitalWrite(PB2PIN,LOW);
  digitalWrite(PB3PIN,LOW);
  digitalWrite(PB4PIN,LOW);
  digitalWrite(PB5PIN,LOW);
  digitalWrite(PB6PIN,LOW);
  digitalWrite(PB7PIN,LOW);
  digitalWrite(FLGPIN,LOW);

// 2022. 2. 4 MZ-1200対策
  delay(1500);

  sdinit();
}

//4BIT受信
byte rcv4bit(void){
//HIGHになるまでループ
  while(digitalRead(CHKPIN) != HIGH){
  }
//受信
  byte j_data = digitalRead(PA0PIN)+digitalRead(PA1PIN)*2+digitalRead(PA2PIN)*4+digitalRead(PA3PIN)*8;
//FLGをセット
  digitalWrite(FLGPIN,HIGH);
//LOWになるまでループ
  while(digitalRead(CHKPIN) == HIGH){
  }
//FLGをリセット
  digitalWrite(FLGPIN,LOW);
  return(j_data);
}

//1BYTE受信
byte rcv1byte(void){
  byte i_data = 0;
  i_data=rcv4bit()*16;
  i_data=i_data+rcv4bit();
  return(i_data);
}

//1BYTE送信
void snd1byte(byte i_data){
//下位ビットから8ビット分をセット
  digitalWrite(PB0PIN,(i_data)&0x01);
  digitalWrite(PB1PIN,(i_data>>1)&0x01);
  digitalWrite(PB2PIN,(i_data>>2)&0x01);
  digitalWrite(PB3PIN,(i_data>>3)&0x01);
  digitalWrite(PB4PIN,(i_data>>4)&0x01);
  digitalWrite(PB5PIN,(i_data>>5)&0x01);
  digitalWrite(PB6PIN,(i_data>>6)&0x01);
  digitalWrite(PB7PIN,(i_data>>7)&0x01);
  digitalWrite(FLGPIN,HIGH);
//HIGHになるまでループ
  while(digitalRead(CHKPIN) != HIGH){
  }
  digitalWrite(FLGPIN,LOW);
//LOWになるまでループ
  while(digitalRead(CHKPIN) == HIGH){
  }
}

//小文字->大文字
char upper(char c){
  if('a' <= c && c <= 'z'){
    c = c - ('a' - 'A');
  }
  return c;
}

//ファイル名の最後が「.mzt」でなければ付加
void addmzt(char *f_name){
  unsigned int lp1=0;
  while (f_name[lp1] != 0x0D){
    lp1++;
  }
  if (f_name[lp1-4]!='.' ||
    ( f_name[lp1-3]!='M' &&
      f_name[lp1-3]!='m' ) ||
    ( f_name[lp1-2]!='Z' &&
      f_name[lp1-2]!='z' ) ||
    ( f_name[lp1-1]!='T' &&
      f_name[lp1-1]!='t' ) ){
         f_name[lp1++] = '.';
         f_name[lp1++] = 'm';
         f_name[lp1++] = 'z';
         f_name[lp1++] = 't';
  }
  f_name[lp1] = 0x00;
}

//ファイル名の最後が「.d88」でなければ付加
void addD88(char *f_name){
  unsigned int lp1=0;
  while (f_name[lp1] != 0x0D){
    lp1++;
  }
  if (f_name[lp1-4]!='.' ||
    ( f_name[lp1-3]!='D' &&
      f_name[lp1-3]!='d' ) ||
    ( f_name[lp1-2]!='8' ) ||
    ( f_name[lp1-1]!='8' ) ){
         f_name[lp1++] = '.';
         f_name[lp1++] = 'd';
         f_name[lp1++] = '8';
         f_name[lp1++] = '8';
  }
  f_name[lp1] = 0x00;
}

//SDカードにSAVE
void f_save(void){
char p_name[20];

//保存ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//プログラムネーム取得
  for (unsigned int lp1 = 0;lp1 <= 16;lp1++){
    p_name[lp1] = rcv1byte();
  }
  p_name[15] =0x0D;
  p_name[16] =0x00;
//スタートアドレス取得
  int s_adrs1 = rcv1byte();
  int s_adrs2 = rcv1byte();
//スタートアドレス算出
  unsigned int s_adrs = s_adrs1+s_adrs2*256;
//エンドアドレス取得
  int e_adrs1 = rcv1byte();
  int e_adrs2 = rcv1byte();
//エンドアドレス算出
  unsigned int e_adrs = e_adrs1+e_adrs2*256;
//実行アドレス取得
  int g_adrs1 = rcv1byte();
  int g_adrs2 = rcv1byte();
//実行アドレス算出
  unsigned int g_adrs = g_adrs1+g_adrs2*256;
//ファイルサイズ算出
  unsigned int f_length = e_adrs - s_adrs + 1;
  unsigned int f_length1 = f_length % 256;
  unsigned int f_length2 = f_length / 256;
//ファイルが存在すればdelete
  if (SD.exists(f_name) == true){
    SD.remove(f_name);
  }
//ファイルオープン
  File file = SD.open( f_name, FILE_WRITE );
  if( true == file ){
//状態コード送信(OK)
    snd1byte(0x00);
//ファイルモード設定(01)
    file.write(char(0x01));
//プログラムネーム
    file.write(p_name);
    file.write(char(0x00));
//ファイルサイズ
    file.write(f_length1);
    file.write(f_length2);
//スタートアドレス
    file.write(s_adrs1);
    file.write(s_adrs2);
//実行アドレス
    file.write(g_adrs1);
    file.write(g_adrs2);
//7Fまで00埋め
    for (unsigned int lp1 = 0;lp1 <= 103;lp1++){
      file.write(char(0x00));
    }
//実データ
    long lp1 = 0;
    while (lp1 <= f_length-1){
      int i=0;
      while(i<=255 && lp1<=f_length-1){
        s_data[i]=rcv1byte();
        i++;
        lp1++;
      }
      file.write(s_data,i);
    }
    file.close();
   } else {
//状態コード送信(ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

//SDカードから読込
void f_load(void){
//ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//ファイルが存在しなければERROR
  if (SD.exists(f_name) == true){
//ファイルオープン
    File file = SD.open( f_name, FILE_READ );
    if( true == file ){
//ファイル種類コードの判別を撤廃
//      if( file.read() == 0x01){
//状態コード送信(OK)
        snd1byte(0x00);
        int wk1 = 0;
        wk1 = file.read();
        for (unsigned int lp1 = 0;lp1 <= 16;lp1++){
          wk1 = file.read();
          snd1byte(wk1);
        }
//ファイルサイズ取得
        int f_length2 = file.read();
        int f_length1 = file.read();
        unsigned int f_length = f_length1*256+f_length2;
//スタートアドレス取得
        int s_adrs2 = file.read();
        int s_adrs1 = file.read();
        unsigned int s_adrs = s_adrs1*256+s_adrs2;
//実行アドレス取得
        int g_adrs2 = file.read();
        int g_adrs1 = file.read();
        unsigned int g_adrs = g_adrs1*256+g_adrs2;
        snd1byte(s_adrs2);
        snd1byte(s_adrs1);
        snd1byte(f_length2);
        snd1byte(f_length1);
        snd1byte(g_adrs2);
        snd1byte(g_adrs1);
        file.seek(128);
//データ送信
        for (unsigned int lp1 = 0;lp1 < f_length;lp1++){
            byte i_data = file.read();
            snd1byte(i_data);
        }
        file.close();
//       } else {
//状態コード送信(ERROR)
//        snd1byte(0xF2);
          sdinit();
//      }  
     } else {
//状態コード送信(ERROR)
      snd1byte(0xFF);
      sdinit();
     }
   } else {
//状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

//ASTART 指定されたファイルをファイル名「0000.mzt」としてコピー
void astart(void){
char w_name[]="0000.mzt";

//ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);
//ファイルが存在しなければERROR
  if (SD.exists(f_name) == true){
//0000.mztが存在すればdelete
    if (SD.exists(w_name) == true){
      SD.remove(w_name);
    }
//ファイルオープン
    File file_r = SD.open( f_name, FILE_READ );
    File file_w = SD.open( w_name, FILE_WRITE );
      if( true == file_r ){
//実データ
        unsigned int f_length = file_r.size();
        long lp1 = 0;
        while (lp1 <= f_length-1){
          int i=0;
          while(i<=255 && lp1<=f_length-1){
            s_data[i]=file_r.read();
            i++;
            lp1++;
          }
          file_w.write(s_data,i);
        }
        file_w.close();
        file_r.close();
//状態コード送信(OK)
        snd1byte(0x00);
      } else {
//状態コード送信(ERROR)
      snd1byte(0xF1);
      sdinit();
    }
  } else {
//状態コード送信(ERROR)
    snd1byte(0xF1);
    sdinit();
  }  
}

// SD-CARDのFILELIST
// type: type (0: mzt, 1:d88)
void dirlist(char type){
  static const char* typeArray[2] = {
    "mzt",
    "d88"
  };
//比較文字列取得 32+1文字まで
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    c_name[lp1] = rcv1byte();
//  Serial.print(c_name[lp1],HEX);
//  Serial.println("");
  }
//
  File file = SD.open( "/" );
  File entry =  file.openNextFile();
  int cntl2 = 0;
  unsigned int br_chk =0;
  int page = 1;
//全件出力の場合には20件出力したところで一時停止、キー入力により継続、打ち切りを選択
  while (br_chk == 0) {
    if(entry){
//    entry.getName(f_name,36);
      entry.getName(s_data, 260);
      memcpy(f_name, s_data, 36);
      unsigned int lp1=0;
//一件送信
//比較文字列でファイルネームを先頭10文字までと拡張子を比較して一致するものだけを出力
      if (f_match(f_name,c_name) && (strcasestr(s_data, typeArray[type]))){
        while (lp1<=36 && f_name[lp1]!=0x00){
        snd1byte(upper(f_name[lp1]));
        lp1++;
        }
        snd1byte(0x0D);
        snd1byte(0x00);
        cntl2++;
      }
    }
    if (!entry || cntl2 > 19){
//継続・打ち切り選択指示要求
      snd1byte(0xfe);

//選択指示受信(0:継続 B:前ページ 以外:打ち切り)
      br_chk = rcv1byte();
//前ページ処理
      if (br_chk==0x42){
//先頭ファイルへ
        file.rewindDirectory();
//entry値更新
        entry =  file.openNextFile();
//もう一度先頭ファイルへ
        file.rewindDirectory();
        if(page <= 2){
//現在ページが1ページ又は2ページなら1ページ目に戻る処理
          page = 0;
        } else {
//現在ページが3ページ以降なら前々ページまでのファイルを読み飛ばす
          page = page -2;
          cntl2=0;
          while(cntl2 < page*20){
            entry =  file.openNextFile();
//          if (upper(f_name[0]) == pg0 || pg0 == 0x20){
            if (f_match(f_name,c_name)){
              cntl2++;
            }
          }
        }
        br_chk=0;
      }
      page++;
      cntl2 = 0;
    }
//ファイルがまだあるなら次読み込み、なければ打ち切り指示
    if (entry){
      entry =  file.openNextFile();
    }else{
      br_chk=1;
    }
//FDLの結果が20件未満なら継続指示要求せずにそのまま終了
    if (!entry && cntl2 < 20 && page ==1){
      break;
    }
  }
//処理終了指示
  snd1byte(0xFF);
  snd1byte(0x00);
}

//f_nameとc_nameをc_nameに0x00が出るまで比較
//FILENAME COMPARE
boolean f_match(char *f_name,char *c_name){
  boolean flg1 = true;
  unsigned int lp1 = 0;
//  Serial.print(f_name);
//  Serial.print(" ");
//  Serial.print(c_name);
//  Serial.print(" ");
  while (lp1 <=32 && c_name[0] != 0x00 && flg1 == true){
//  Serial.print(f_name[lp1],HEX);
//  Serial.print("-");
//  Serial.print(c_name[lp1+1],HEX);
//  Serial.print(" ");
    if (upper(f_name[lp1]) != c_name[lp1+1]){
      flg1 = false;
    }
    lp1++;
    if (c_name[lp1+1]==0x00){
      break;
    }
  }
//  if (flg1){
//    Serial.println("true");
//  }else{
//    Serial.println("false");
//  }
  return flg1;
}

//FILE DELETE
void f_del(void){

//ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);

//ファイルが存在しなければERROR
  if (SD.exists(f_name) == true){
//状態コード送信(OK)
    snd1byte(0x00);

//処理選択を受信(0:継続してDELETE 0以外:CANSEL)
    if (rcv1byte() == 0x00){
      if (SD.remove(f_name) == true){
//状態コード送信(OK)
        snd1byte(0x00);
      }else{
//状態コード送信(Error)
        snd1byte(0xf1);
        sdinit();
      }
    } else{
//状態コード送信(Cansel)
      snd1byte(0x01);
    }
  }else{
//状態コード送信(Error)
        snd1byte(0xf1);
        sdinit();
  }
}

//FILE RENAME
void f_ren(void){

//現ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);

//ファイルが存在しなければERROR
  if (SD.exists(f_name) == true){
//状態コード送信(OK)
    snd1byte(0x00);

//新ファイルネーム取得
    for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
      new_name[lp1] = rcv1byte();
    }
    addmzt(new_name);
//状態コード送信(OK)
    snd1byte(0x00);

    File file = SD.open( f_name, FILE_WRITE );
    if( true == file ){
      if (file.rename(new_name)){
 //状態コード送信(OK)
         snd1byte(0x00);
        } else {
 //状態コード送信(OK)
          snd1byte(0xff);
        }
      file.close();
    }else{
//状態コード送信(Error)
      snd1byte(0xf1);
      sdinit();
    }
  }else{
//状態コード送信(Error)
      snd1byte(0xf1);
      sdinit();
  }
}

//FILE DUMP
void f_dump(void){
unsigned int br_chk =0;

//ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  addmzt(f_name);

//ファイルが存在しなければERROR
  if (SD.exists(f_name) == true){
//状態コード送信(OK)
    snd1byte(0x00);

//ファイルオープン
    File file = SD.open( f_name, FILE_READ );
      if( true == file ){
//実データ送信(1画面:128Byte)
        unsigned int f_length = file.size();
        long lp1 = 0;
        while (lp1 <= f_length-1){
//画面先頭ADRSを送信
          snd1byte(lp1 % 256);
          snd1byte(lp1 / 256);
          int i=0;
//実データを送信
          while(i<128 && lp1<=f_length-1){
            snd1byte(file.read());
            i++;
            lp1++;
          }
//FILE ENDが128Byteに満たなかったら残りByteに0x00を送信
          while(i<128){
            snd1byte(0x00);
            i++;
          }
//指示待ち
          br_chk=rcv1byte();
//BREAKならポインタをFILE ENDとする
          if (br_chk==0xff){
            lp1 = f_length; 
          }
//B:BACKを受信したらポインタを256Byte戻す。先頭画面なら0に戻してもう一度先頭画面表示
          if (br_chk==0x42){
            if(lp1>255){
              if (lp1 % 128 == 0){
                lp1 = lp1 - 256;
              } else {
                lp1 = lp1 - 128 - (lp1 % 128);
              }
              file.seek(lp1);
            } else{
              lp1 = 0;
              file.seek(0);
            }
          }
        }
//FILE ENDもしくはBREAKならADRSに終了コード0FFFFHを送信
        if (lp1 > f_length-1){
          snd1byte(0xff);
          snd1byte(0xff);
        };
        file.close();
//状態コード送信(OK)
        snd1byte(0x00);
      } else {
//状態コード送信(ERROR)
      snd1byte(0xF1);
      sdinit();
    }
  }else{
//状態コード送信(Error)
        snd1byte(0xf1);
        sdinit();
  }
}

//FILE COPY
void f_copy(char type){

//現ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    f_name[lp1] = rcv1byte();
  }
  if (type == 0) {
    addmzt(f_name);
  } else {
    addD88(f_name);
  }
//ファイルが存在しなければERROR
  if (SD.exists(f_name) == true){
//状態コード送信(OK)
    snd1byte(0x00);

//新ファイルネーム取得
    for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
      new_name[lp1] = rcv1byte();
    }
    if (type == 0) {
      addmzt(new_name);
    } else {
      addD88(new_name);
    }
//新ファイルネームと同じファイルネームが存在すればERROR
    if (SD.exists(new_name) == false){
//状態コード送信(OK)
        snd1byte(0x00);
//ファイルオープン
    File file_r = SD.open( f_name, FILE_READ );
    File file_w = SD.open( new_name, FILE_WRITE );
      if( true == file_r ){
//実データコピー
        unsigned int f_length = file_r.size();
        long lp1 = 0;
        while (lp1 <= f_length-1){
          int i=0;
          while(i<=255 && lp1<=f_length-1){
            s_data[i]=file_r.read();
            i++;
            lp1++;
          }
          file_w.write(s_data,i);
        }
        file_w.close();
        file_r.close();
//状態コード送信(OK)
        snd1byte(0x00);
      }else{
//状態コード送信(Error)
      snd1byte(0xf1);
      sdinit();
    }
      }else{
//状態コード送信(Error)
        snd1byte(0xf3);
        sdinit();
    }
  }else{
//状態コード送信(Error)
      snd1byte(0xf1);
      sdinit();
  }
}

//91hで0436H MONITOR ライト インフォメーション代替処理
void mon_whead(void){
char m_info[130];
//インフォメーションブロック受信
  for (unsigned int lp1 = 0;lp1 < 128;lp1++){
    m_info[lp1] = rcv1byte();
  }
//S-OS SWORDからは最後が20hのファイルネームが送られて来るため0dhを付加
//8080用テキスト・エディタ＆アセンブラからファイルネームの後ろに20hが送られて来るため0dhに修正
//MZ-80K側で対処
//  int lp2 = 17;
//  while (lp2>0 && (m_info[lp2] ==0x20 || m_info[lp2] ==0x0d)){
//    m_info[lp2]=0x0d;
//    lp2--;
//  }
//ファイルネーム取り出し
  for (unsigned int lp1 = 0;lp1 < 17;lp1++){
    m_name[lp1] = m_info[lp1+1];
  }
//DOSファイルネーム用に.MZTを付加
  addmzt(m_name);
  m_info[16] = 0x0d;
//ファイルが存在すればdelete
  if (SD.exists(m_name) == true){
    SD.remove(m_name);
  }
//ファイルオープン
  File file = SD.open( m_name, FILE_WRITE );
  if( true == file ){
//状態コード送信(OK)
    snd1byte(0x00);
//インフォメーションブロックwrite
    for (unsigned int lp1 = 0;lp1 < 128;lp1++){
      file.write(m_info[lp1]);
    }
    file.close();
  } else {
//状態コード送信(ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

//92hで0475H MONITOR ライト データ代替処理
void mon_wdata(void){
//ファイルサイズ取得
  int f_length1 = rcv1byte();
  int f_length2 = rcv1byte();
//ファイルサイズ算出
  unsigned int f_length = f_length1+f_length2*256;
//ファイルオープン
  File file = SD.open( m_name, FILE_WRITE );
  if( true == file ){
//状態コード送信(OK)
    snd1byte(0x00);
//実データ
    long lp1 = 0;
    while (lp1 <= f_length-1){
      int i=0;
      while(i<=255 && lp1<=f_length-1){
        s_data[i]=rcv1byte();
        i++;
        lp1++;
      }
      file.write(s_data,i);
    }
    file.close();
  } else {
//状態コード送信(ERROR)
    snd1byte(0xF1);
  }
}

//04D8H MONITOR リード インフォメーション代替処理
void mon_lhead(void){
//リード データ POINTクリア
  m_lop=128;
//ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    m_name[lp1] = rcv1byte();
  }
  addmzt(m_name);
//ファイルが存在しなければERROR
  if (SD.exists(m_name) == true){
    snd1byte(0x00);
//ファイルオープン
    File file = SD.open( m_name, FILE_READ );
    if( true == file ){
      snd1byte(0x00);
      for (unsigned int lp1 = 0;lp1 < 128;lp1++){
          byte i_data = file.read();
          snd1byte(i_data);
      }
      file.close();
      snd1byte(0x00);
    } else {
//状態コード送信(ERROR)
      snd1byte(0xFF);
      sdinit();
    }  
  } else {
//状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

//04F8H MONITOR リード データ代替処理
void mon_ldata(void){
  addmzt(m_name);
//ファイルが存在しなければERROR
  if (SD.exists(m_name) == true){
    snd1byte(0x00);
//ファイルオープン
    File file = SD.open( m_name, FILE_READ );
    if( true == file ){
      snd1byte(0x00);
      file.seek(m_lop);
//読み出しサイズ取得
      int f_length2 = rcv1byte();
      int f_length1 = rcv1byte();
      unsigned int f_length = f_length1*256+f_length2;
      for (unsigned int lp1 = 0;lp1 < f_length;lp1++){
        byte i_data = file.read();
        snd1byte(i_data);
      }
      file.close();
      m_lop=m_lop+f_length;
      snd1byte(0x00);
    } else {
//状態コード送信(ERROR)
      snd1byte(0xFF);
    }  
  } else {
//状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

//BOOT処理(MZ-2000_SD専用)
void boot(void){
//ファイルネーム取得
  for (unsigned int lp1 = 0;lp1 <= 32;lp1++){
    m_name[lp1] = rcv1byte();
  }
////  Serial.print("m_name:");
////  Serial.println(m_name);
//ファイルが存在しなければERROR
  if (SD.exists(m_name) == true){
    snd1byte(0x00);
//ファイルオープン
    File file = SD.open( m_name, FILE_READ );
    if( true == file ){
    snd1byte(0x00);
//ファイルサイズ送信
      unsigned long f_length = file.size();
      unsigned int f_len1 = f_length / 256;
      unsigned int f_len2 = f_length % 256;
      snd1byte(f_len2);
      snd1byte(f_len1);
////  Serial.println(f_length,HEX);
////  Serial.println(f_len2,HEX);
////  Serial.println(f_len1,HEX);

//実データ送信
      for (unsigned long lp1 = 1;lp1 <= f_length;lp1++){
         byte i_data = file.read();
         snd1byte(i_data);
      }

    } else {
//状態コード送信(ERROR)
      snd1byte(0xFF);
    }  
  } else {
//状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
  }
}

// 連結ファイルオープン
// 0xE0, ファイルネーム(33bytes)
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:ERROR
void ConcatFileOpen()
{
  if(isConcatState == 1)
  {
    concatFile.close();
  }
  // ファイルネーム取得
  for (unsigned int lp1 = 0; lp1 <= 32; lp1 ++) {
    concatName[lp1] = rcv1byte();
  }
  addmzt(concatName);
  // ファイルが存在するか、しなければERROR
  if (SD.exists(concatName) == true) {
    //ファイルオープン
    concatFile = SD.open(concatName, FILE_READ);
    if ( true == concatFile ) {
      concatSize = concatFile.size();
      //状態コード送信(OK)
      snd1byte(0x00);
      isConcatState = 1; // オープンしている
      concatPos = 0;
    } else {
      //状態コード送信(ERROR)
      snd1byte(0xFF);
      sdinit();
    }
  } else {
    // 状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

// 連結ファイルを1ブロック読み込む
// 0xE1
// Result:    17bytes: MZファイル名
//            2bytes:  読み込みアドレス
//            2bytes:  データサイズ
//            2bytes:  実行アドレス
// データサイズbytes:  データ
//            1byte:   ステータス: 0xFE:次のデータがある, 0x00次のデータが無い(終了)
void ConcatFileRead()
{
  if(isConcatState == 0)
  {
    // オープンしていない
    for (unsigned int i = 0; i < 17; i ++) {
      snd1byte(0);
    }
    snd1byte(0xB1);
    for (unsigned int i = 0; i < 6; i ++) {
      snd1byte(0);
    }
    return;
  }
  // モード読み捨て
  int wk1 = concatFile.read();
  // ファイル名
  for (unsigned int lp1 = 0;lp1 <= 16; lp1 ++) {
    wk1 = concatFile.read();
    snd1byte(wk1);
  }
  //データサイズ取得
  int f_length2 = concatFile.read();
  int f_length1 = concatFile.read();
  unsigned int f_length = f_length1*256+f_length2;
  //スタートアドレス取得
  int s_adrs2 = concatFile.read();
  int s_adrs1 = concatFile.read();
  unsigned int s_adrs = s_adrs1*256+s_adrs2;
  //実行アドレス取得
  int g_adrs2 = concatFile.read();
  int g_adrs1 = concatFile.read();
  unsigned int g_adrs = g_adrs1*256+g_adrs2;
  snd1byte(s_adrs2);
  snd1byte(s_adrs1);
  snd1byte(f_length2);
  snd1byte(f_length1);
  snd1byte(g_adrs2);
  snd1byte(g_adrs1);
  concatPos += 128;
  concatFile.seek(concatPos);
  //データ送信
  for (unsigned int lp1 = 0;lp1 < f_length; lp1 ++) {
    byte i_data = concatFile.read();
    snd1byte(i_data);
    ++ concatPos;
  }
  if (concatPos < concatSize) {
    // 次のデータがある
    snd1byte(0xFE);
  } else {
    // 次のデータが無い
    snd1byte(0x00);
  }
}

// 連結ファイルを1ブロックスキップ
// 0xE2
// Result: 0xFE:次のデータがある, 0x00:次のデータが無い(終了), 0xFF:オープンしていない
void ConcatFileSkip()
{
  if(isConcatState == 0)
  {
    // オープンしていない
    snd1byte(0xFF);
    sdinit();
    return;
  }
  // モード読み捨て
  int wk1 = concatFile.read();
  // ファイル名
  for(unsigned int lp1 = 0;lp1 <= 16; lp1 ++) {
    wk1 = concatFile.read();
  }
  //データサイズ取得
  int f_length2 = concatFile.read();
  int f_length1 = concatFile.read();
  unsigned int f_length = f_length1*256+f_length2;
  concatPos += (128 + f_length);
  concatFile.seek(concatPos);
  if (concatPos < concatSize) {
    // 次のデータがある
    snd1byte(0xFE);
  } else {
    // 次のデータが無い
    snd1byte(0x00);
  }
}

// 連結ファイルからブロックを探す
// 0xE3, MZファイルネーム(17bytes)
// Result: 0x00:OK, 0xF1:FILE NOT FIND, 0xFF:オープンしていない
void ConcatFileFind()
{
  if(isConcatState == 0) {
    snd1byte(0xFF);
    sdinit();
    return;
  }
  unsigned long concatPosBackup = concatPos;
  // ファイル名
  for (unsigned int lp1 = 0;lp1 < 17; lp1 ++) {
    f_name[lp1] = rcv1byte();
    if(f_name[lp1] == 0x0D) {
      f_name[lp1] = 0;
    }
  }
  f_name[17] = 0;
  snd1byte(0x00);
  // 検索する
  while(1) {
    // モード読み捨て
    int wk1 = concatFile.read();
    // ファイル名
    for (unsigned int lp1 = 0;lp1 < 17; lp1 ++) {
      m_name[lp1] = concatFile.read();
      if(m_name[lp1] == 0x0D) {
        m_name[lp1] = 0;
      }
    }
    m_name[17] = 0;
    //データサイズ取得
    int f_length2 = concatFile.read();
    int f_length1 = concatFile.read();
    unsigned int f_length = f_length1*256+f_length2;
    if(String(f_name).equals(m_name) == true) {
      // 見つけた
      concatFile.seek(concatPos);
      break;
    } else {
      // 次のファイル
      concatPos += (128 + f_length);
      if (concatPos >= concatSize) {
        // 見つからなかった
        concatFile.seek(concatPosBackup);
        snd1byte(0xF1);
        return;
      }
      concatFile.seek(concatPos);
    }
  }
  snd1byte(0x00);
}

// 連結ファイルのトップに戻る
// 0xE4
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:オープンしていない
void ConcatFileTop()
{
  if(isConcatState == 0) {
    snd1byte(0xFF);
    sdinit();
    return;
  }
  concatPos = 0;
  concatFile.seek(concatPos);
  snd1byte(0x00);
}

// 連結ファイルクローズ
// 0xE5
// Result: 0x00:OK, 0xF1:FILE NOT FIND ERROR, 0xFF:オープンしていない
void ConcatFileClose()
{
  if(isConcatState == 0) {
    snd1byte(0xFF);
    sdinit();
    return;
  } else if(isConcatState == 1) {
    concatFile.close();
  }
  isConcatState = 0;
  snd1byte(0x00);
}

// 連結ファイルの次のデータがあるか
// 0xE6
// Result: 0xFE:次のデータがある, 0x00:次のデータが無い(終了) 0xFF:オープンしていない
void ConcatFileState(void)
{
  if(isConcatState == 0) {
    // オープンしていない
    snd1byte(0xFF);
    sdinit();
  } else if (concatPos < concatSize) {
    // 次のデータがある
    snd1byte(0xFE);
  } else {
    // 次のデータが無い
    snd1byte(0x00);
  }
}

// MIDIコマンド送信
// 0xE7, 送信バイト数, 送信データ
// Result:  0x00:エラー無し, 0xFF:エラー
void SendMidi(void)
{
  byte length = rcv1byte();
  for(byte i = 0; i < length; i ++)
  {
    byte data = rcv1byte();
    Serial.write(data);
  }
}

// D88関連ファイル処理
bool Open(const char* path)
{
	d88File = SD.open(path, O_READ | O_APPEND);
	if(d88File != true)
	{
		return false;
	}
	return true;
}

void Close(void)
{
  d88File.close();
}

unsigned char Read(void)
{
	unsigned char buffer = (unsigned char)d88File.read();
	return buffer;
}

void Write(unsigned char data)
{
	d88File.write(data);
}

void Seek(int pos)
{
	d88File.seek((unsigned long)pos);
}

// D88関連イメージ処理
bool D88Open(const char* path, bool r)
{
	if(Open(path) == false)
	{
		return false;
	}
	D88Seek(0, 1);
	sectorsPerTrack = numberOfSector;
	reverse = r;
	return true;
}

void D88Close(void)
{
	Close();
}

void D88SetSectorsPerTrack(short num)
{
	sectorsPerTrack = num;
}

// track:  0 -
// sector: 1 -
bool D88Seek(char track, char sector)
{
	int cyl = track / 2;
	int hed = track % 2;
	if(reverse == true)
	{
		hed = 1 - hed;
	}
	int seekTrack = cyl * 2 + hed;
	int seek = 32 + seekTrack * 4;
	unsigned char buffer;
	Seek(seek);
	buffer = Read();
	int offset = (int)buffer;
	buffer = Read();
	offset = offset | buffer << 8;
	buffer = Read();
	offset = offset | buffer << 16;
	buffer = Read();
	offset = offset | buffer << 24;
	// offset: start of track
	int i = 1;
	for(;;)
	{
		// search sector
		Seek(offset);
		c = Read();
		h = Read();
		r = Read();
		n = Read();
		buffer = Read();
		numberOfSector = buffer;
		buffer = Read();
		numberOfSector = numberOfSector | buffer << 8;
		buffer = Read();
		buffer = Read();
		buffer = Read();
		buffer = Read();
		buffer = Read();
		buffer = Read();
		buffer = Read();
		buffer = Read();
		buffer = Read();
		sizeOfData = buffer;
		buffer = Read();
		sizeOfData = sizeOfData | buffer << 8;
		if(r == sector)
		{
			break;
		}
		++ i;
		if(i >= numberOfSector)
		{
			return false;
		}
		offset += 16;
		offset += sizeOfData;
	}
	seekInfoPointer = offset;
	seekDataPointer = offset + 16;
	// seek data area
	Seek(seekDataPointer);
	seekDataOffset = 0;
	return true;
}

bool D88SeekLba(int lba)
{
	int track = lba / sectorsPerTrack;
	int sector = lba % sectorsPerTrack + 1;
	return D88Seek(track, sector);
}

int D88GetSectorSize(void)
{
	return (int)sizeOfData;
}

unsigned char D88Read(void)
{
	if(seekDataOffset >= sizeOfData)
	{
		return 0;
	}
	unsigned char buffer = Read();
	++ seekDataOffset;
	if(reverse == true)
	{
		buffer = buffer ^ 255;
	}
	return buffer;
}

void D88Write(unsigned char data)
{
	if(seekDataOffset >= sizeOfData)
	{
		return;
	}
	Write(data);
	++ seekDataOffset;
}

// D88読み込みOpen
// 0E9h
void d88OpenRead(void)
{
  // ファイルネーム取得
  for (unsigned int lp1 = 0; lp1 <= 32; lp1 ++)
  {
    d88Name[lp1] = rcv1byte();
  }
  addD88(d88Name);
  //ファイルオープン
  if(D88Open(d88Name, true) == true)
  {
    //状態コード送信(OK)
    snd1byte(0x00);
    isD88State = 1; // オープンしている
  }
  else
  {
    // 状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

// D88書き込みOpen
// 0EAh
void d88OpenWrite(void)
{
  // ファイルネーム取得
  for (unsigned int lp1 = 0; lp1 <= 32; lp1 ++)
  {
    d88Name[lp1] = rcv1byte();
  }
  addD88(d88Name);
  //ファイルオープン
  if(D88Open(d88Name, true) == true)
  {
    //状態コード送信(OK)
    snd1byte(0x00);
    isD88State = 1; // オープンしている
  }
  else
  {
    // 状態コード送信(FILE NOT FIND ERROR)
    snd1byte(0xF1);
    sdinit();
  }
}

// D88Close
// 0EBh
void d88Close(void)
{
  if(isD88State == 0)
  {
    snd1byte(0xFF);
    return;
  }
  else
  {
    D88Close();
  }
  isD88State = 0;
  snd1byte(0x00);
}

// D88セクタ読み込み
// 0ECh
void d88ReadLba(void)
{
  unsigned short lba = rcv1byte();
  lba |= rcv1byte() * 256;
  if(isConcatState == 0)
  {
    snd1byte(0xFF);
    return;
  }
  D88SeekLba((int)lba);
  for(int i = 0; i < 256; ++ i)
  {
    unsigned char data = D88Read();
    snd1byte(data);
  }
  // 転送終了
  snd1byte(0x00);
}

// D88セクタ書き込み
// 0EDh
void d88WriteLba(void)
{
  unsigned short lba = rcv1byte();
  lba |= rcv1byte() * 256;
  if(isConcatState == 0)
  {
    snd1byte(0xFF);
    return;
  }
  D88SeekLba((int)lba);
  for(int i = 0; i < 256; ++ i)
  {
    unsigned char data = rcv1byte();
    // todo セクタへの書き込み
    D88Write(data);
  }
  // 書き込み終了
  snd1byte(0x00);
}

// メインループ
void loop()
{
  digitalWrite(PB0PIN,LOW);
  digitalWrite(PB1PIN,LOW);
  digitalWrite(PB2PIN,LOW);
  digitalWrite(PB3PIN,LOW);
  digitalWrite(PB4PIN,LOW);
  digitalWrite(PB5PIN,LOW);
  digitalWrite(PB6PIN,LOW);
  digitalWrite(PB7PIN,LOW);
  digitalWrite(FLGPIN,LOW);
//コマンド取得待ち
////  Serial.print("cmd:");
  byte cmd = rcv1byte();
////  Serial.println(cmd,HEX);
  if((cmd < 0xE0) && (isConcatState == 1))
  {
    // 連結ファイルオープン中に通常ファイルコマンドが来たので連結ファイルはクローズする
    concatFile.close();
    isConcatState = 0;
  }
  if (eflg == false){
    switch(cmd) {
//80hでSDカードにsave
      case 0x80:
////  Serial.println("SAVE START");
//状態コード送信(OK)
        snd1byte(0x00);
        f_save();
        break;
//81hでSDカードからload
      case 0x81:
////  Serial.println("LOAD START");
//状態コード送信(OK)
        snd1byte(0x00);
        f_load();
        break;
//82hで指定ファイルを0000.mztとしてリネームコピー
      case 0x82:
////  Serial.println("ASTART START");
//状態コード送信(OK)
        snd1byte(0x00);
        astart();
        break;
//83hでファイルリスト出力
      case 0x83:
////  Serial.println("FILE LIST START");
//状態コード送信(OK)
        snd1byte(0x00);
        sdinit();
        dirlist(0);
        break;
//84hでファイルDelete
      case 0x84:
////  Serial.println("FILE Delete START");
//状態コード送信(OK)
        snd1byte(0x00);
        f_del();
        break;
//85hでファイルリネーム
      case 0x85:
////  Serial.println("FILE Rename START");
//状態コード送信(OK)
        snd1byte(0x00);
        f_ren();
        break;
      case 0x86:  
//86hでファイルダンプ
////  Serial.println("FILE Dump START");
//状態コード送信(OK)
        snd1byte(0x00);
        f_dump();
        break;
      case 0x87:  
//87hでファイルコピー
////  Serial.println("FILE Copy START");
//状態コード送信(OK)
        snd1byte(0x00);
        f_copy(0);
        break;
      case 0x91:
//91hで0436H MONITOR ライト インフォメーション代替処理
////  Serial.println("0436H START");
//状態コード送信(OK)
        snd1byte(0x00);
        mon_whead();
        break;
//92hで0475H MONITOR ライト データ代替処理
      case 0x92:
////  Serial.println("0475H START");
//状態コード送信(OK)
        snd1byte(0x00);
        mon_wdata();
        break;
//93hで04D8H MONITOR リード インフォメーション代替処理
      case 0x93:
////  Serial.println("04D8H START");
//状態コード送信(OK)
        snd1byte(0x00);
        mon_lhead();
        break;
//94hで04F8H MONITOR リード データ代替処理
      case 0x94:
////  Serial.println("04F8H START");
//状態コード送信(OK)
        snd1byte(0x00);
        mon_ldata();
        break;
//95hでBOOT LOAD(MZ-2000_SD専用)
      case 0x95:
////  Serial.println("BOOT LOAD START");
//状態コード送信(OK)
        snd1byte(0x00);
        boot();
        break;

//連結ファイルコマンド
//0E0hで連結ファイルオープン
      case 0xE0:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileOpen();
        break;
//0E1hで連結ファイル1ブロック読み込み
      case 0xE1:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileRead();
        break;
//0E2hで連結ファイルを1ブロックスキップ
      case 0xE2:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileSkip();
        break;
//0E3hで連結ファイルからブロックを探す
      case 0xE3:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileFind();
        break;
//0E4hで連結ファイルのトップに戻る
      case 0xE4:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileTop();
        break;
//0E5hで連結ファイルクローズ
      case 0xE5:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileClose();
        break;
//0E6hで連結ファイルの次のデータがあるか
      case 0xE6:
//状態コード送信(OK)
        snd1byte(0x00);
        ConcatFileState();
        break;

//0E7hでMIDIコマンド送信
      case 0xE7:
//状態コード送信(OK)
        snd1byte(0x00);
        SendMidi();
        break;

// 0E8hでD88ファイル一覧
      case 0xE8:
        snd1byte(0x00);
        dirlist(1);
        break;

// 0E9hでD88ファイルを読み込みモードでオープン
      case 0xE9:
        snd1byte(0x00);
        d88OpenRead();
        break;

// 0EAhでD88ファイルを書き込みモードでオープン
      case 0xEA:
        snd1byte(0x00);
        d88OpenWrite();
        break;

// 0EBhでD88ファイルを閉じる
      case 0xEB:
        snd1byte(0x00);
        d88Close();
        break;

// 0EChでD88ファイルのセクタデータをLBA指定で読み込む
      case 0xEC:
        snd1byte(0x00);
        d88ReadLba();
        break;

// 0EDhでD88ファイルのセクタデータをLBA指定で書き込む
      case 0xED:
        snd1byte(0x00);
        d88WriteLba();
        break;

// 0EEhでD88ファイルファイルコピー
      case 0xEE:
        snd1byte(0x00);
        f_copy(1);
        break;

      default:
//状態コード送信(CMD ERROR)
        snd1byte(0xF4);
    }
  } else {
//状態コード送信(ERROR)
    snd1byte(0xF0);
    sdinit();
  }
}
