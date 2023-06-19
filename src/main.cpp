// 由于某些不可抗拒因素，WS2812B 的灯珠颜色有所差异，以下是实际对应
// r -> 绿色，g -> 红色， b -> 蓝色

#include <Arduino.h>
#include <FastLED.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// ----------------常量定义----------------

// 最大歌曲数
#define NUM_SONG 6
// WS2812B 灯珠行列数量
#define COLS 7
#define ROWS 8
// WS2812B 灯珠数量
#define NUM_LEDS 56
// LED 映射矩阵数组
const int ledmap[COLS][ROWS] = {
    { 7,  6,  5,  4,  3,  2,  1,  0},
    { 8,  9, 10, 11, 12, 13, 14, 15},
    {23, 22, 21, 20, 19, 18, 17, 16},
    {24, 25, 26, 27, 28, 29, 30, 31},
    {39, 38, 37, 36, 35, 34, 33, 32},
    {40, 41, 42, 43, 44, 45, 46, 47},
    {55, 54, 53, 52, 51, 50, 49, 48},
};

const CRGB level_rgb[9][8] = {
    {CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB(  0, 30,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB(  5, 50,  0),CRGB( 10, 70,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB( 15, 90,  0),CRGB( 20,110,  0),CRGB( 30,130,  5),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB( 50,150, 10),CRGB( 70,170, 15),CRGB( 90,190, 20),CRGB(110,210, 25),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB(130,230, 10),CRGB(150,250, 10),CRGB(170,230, 10),CRGB(190,210, 10),CRGB(210,190, 10),CRGB(  0,  0,  0),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB(230,170, 30),CRGB(250,150, 50),CRGB(230,130, 70),CRGB(210,110, 90),CRGB(190, 90,110),CRGB(170, 70,130),CRGB(  0,  0,  0),CRGB(  0,  0,  0)},
    {CRGB(150, 50,150),CRGB(130, 30,170),CRGB(110, 50,190),CRGB( 90, 70,210),CRGB( 70, 90,230),CRGB( 50,110,250),CRGB( 30,130,230),CRGB(  0,  0,  0)},
    {CRGB( 50,150,210),CRGB( 70,170,190),CRGB( 90,190,170),CRGB(110,210,150),CRGB(130,230,130),CRGB(150,250,110),CRGB(170,230, 90),CRGB(190,210, 70)},
};

// 每个音调对应的数字
const int tone_id[8] = {261, 293, 329, 349, 391, 440, 493, 547};

// ----------------引脚设置----------------

// 与 DFPlayer 通信的软串口 RX, TX
SoftwareSerial mySoftwareSerial(11, 10); 
// WS2812B 数据引脚
#define DATA_PIN 9
// 模式指示灯
#define MOD_LIGHT 2
// 换歌按钮
#define CHSONG_BUTTON 3
// 播放器/电子琴转换按钮
#define CHMOD_BUTTON 4
// 摇杆输出
#define ROCKER_VRx A1
#define ROCKER_VRy A2
#define ROCKER_SW 5 // useless
// MSGEQ7 相关引脚
#define MSGEQ_OUT A0 // 3 pin
#define MSGEQ_RESET 13 // 7 pin
#define MSGEQ_STROBE 12 // 4 pin

// ----------------全局变量定义-----------------

// 播放器/电子琴模式，默认为1，表示电子琴
int mod = 1;
// 歌曲曲目, 默认为 1
int song_id = 1;
// 前一个音调，-1 表示无效
int pre_tone = -1;
// MSGEQ7 输出的 7 个强度
int level[7];
// WS2812B 颜色数组
CRGB leds[NUM_LEDS];

// ---------------主要的函数声明-----------------

// 下一首歌
void next_song();
// 改变运行模式
void change_mod();
// 读取 MSGEQ7 的输出到 level 数组
void read_MSGEQ7();
// 根据摇杆得到音调, -1 为无效
int get_tone();
// 显示 LED 频谱
void LED_show();

// ----------------------------------------------


// DFPlayer 定义
DFRobotDFPlayerMini myDFPlayer;

// 测试模式
#define TESTMOD 3

void setup() {

    mySoftwareSerial.begin(9600);
    Serial.begin(115200, SERIAL_8E2);
    
    // 初始化 MSGEQ7
    pinMode(MSGEQ_STROBE, OUTPUT);
    pinMode(MSGEQ_RESET,  OUTPUT);
    pinMode(MSGEQ_OUT, INPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(MOD_LIGHT, OUTPUT);

    digitalWrite(MSGEQ_RESET, LOW);
    digitalWrite(MSGEQ_STROBE, HIGH);

    // 使用外部中断似乎有些问题
    // attachInterrupt(0, change_mod, RISING);
    // attachInterrupt(1, next_song, RISING);

    // LED 初始化
    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(10); // 设置亮度

    // ------------------ LED 开机测试 ----------------------------
    for(int i = 0; i < NUM_LEDS; i ++) {
        leds[i] = CRGB::Green;
    }
    FastLED.show();
    delay(500);
    for(int i = 0; i < NUM_LEDS; i ++) {
        leds[i] = CRGB::Red;
    }
    FastLED.show();
    delay(500);
    for(int i = 0; i < NUM_LEDS; i ++) {
        leds[i] = CRGB::Blue;
    }
    FastLED.show();
    delay(500);
    FastLED.clear();
    FastLED.show();
    // -----------------------------------------------------------

    // 初始化 DFPlaer
    if (!myDFPlayer.begin(mySoftwareSerial)) {
        Serial.println(F("DFPlayer unable to begin"));
        while(true);
    }
    Serial.println(F("DFPlayer Mini online.")); 

    myDFPlayer.volume(24);  // 设置音量 From 0 to 30

}

void loop() {
    change_mod();
    switch (mod) {
    case 0:
        digitalWrite(MOD_LIGHT, HIGH);
        next_song();
        break;
    case 1:
        digitalWrite(MOD_LIGHT, LOW);
        int tone = get_tone();
        if(tone != pre_tone) {
            delay(20);
            int testtone = get_tone();
            if(testtone != tone) {
                break;
            }
            if(tone != -1) {
                myDFPlayer.playMp3Folder(tone_id[tone]);
            }
            pre_tone = tone;
#if 1 == TESTMOD
            Serial.println(tone_id[tone]);
#endif
        }
        break;
    }
    read_MSGEQ7();
    LED_show();

}

void change_mod() {
    if(digitalRead(CHMOD_BUTTON) == LOW) return;
    delay(100);
    if(digitalRead(CHMOD_BUTTON) == HIGH) {
        mod ^= 1;
        myDFPlayer.stop();
        if(!mod) {
            delay(10);
            song_id = 1;
            myDFPlayer.playMp3Folder(song_id);
            Serial.println(song_id);
        } else {
            delay(10);
            pre_tone = -1;
        }
    }
}

void next_song() {
    if(digitalRead(CHSONG_BUTTON) == LOW) return;
    delay(100);
    if(digitalRead(CHSONG_BUTTON) == HIGH) {
        song_id = song_id % NUM_SONG + 1;
        myDFPlayer.playMp3Folder(song_id);
        Serial.println(song_id);
    }
}

int get_tone() {
    int x = analogRead(ROCKER_VRx);
    int y = analogRead(ROCKER_VRy);
#if 2 == TESTMOD
    Serial.print(F("x:")), Serial.println(x);
    Serial.print(F("y:")), Serial.println(y);
#endif 
    if(y <= 341) {
        if(x < 0) {
            return -1;
        } else if(x <= 341) {
            return 0;
        } else if(x <= 682) {
            return 1;
        } else if(x <= 1023) {
            return 2;
        } else {
            return -1;
        }
    } else if(y <= 682) {
        if(x < 0) {
            return -1;
        } else if(x <= 341) {
            return 7;
        } else if(x <= 682) {
            return -1;
        } else if(x <= 1023) {
            return 3;
        } else {
            return -1;
        }
    } else if(y <= 1023) {
        if(x < 0) {
            return -1;
        } else if(x <= 341) {
            return 6;
        } else if(x <= 682) {
            return 5;
        } else if(x <= 1023) {
            return 4;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

void read_MSGEQ7() {
    digitalWrite(MSGEQ_RESET, HIGH);
    digitalWrite(MSGEQ_RESET, LOW);
    delayMicroseconds(75);

    for (int i = 0; i < 7; i ++) {
        digitalWrite(MSGEQ_STROBE, LOW);
        delayMicroseconds(40); // Delay  necessary due to timing diagram, 源代码中是 100
        level[i] = analogRead(MSGEQ_OUT);

#if 3 == TESTMOD
            Serial.print(level[i]);
            Serial.print(F(" "));
#endif
        digitalWrite(MSGEQ_STROBE, HIGH);
        delayMicroseconds(40); // Delay  necessary due to timing diagram, 源代码中是 100
    }
#if 3 == TESTMOD
    Serial.println("");
#endif

}

void LED_show() {
    for(int j = ROWS - 1; j >= 0 ; j --) {
        for(int i = 0; i < COLS; i ++) {
            int x = level[i];
            if(x <= 800) x = 1600 - x;
            x = (x - 800) / 18;
            if(x < 0) x = 0;
            if(x > 8) x = 8;
            leds[ledmap[i][j]] = level_rgb[x][j];
        }
        FastLED.show();
        delay(1);
    }
}