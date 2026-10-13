/*
  程序功能：接收串口数据帧，控制 GPIO 48 的无源蜂鸣器切换播放两首歌曲。
  使用方法：串口设为 115200 和 HEX；发送 55 05 02 01 08 FF 或 55 05 02 02 09 FF。
  运行结果：分别播放《两只老虎》或《小星星》，并返回对应的 0x82 应答帧。
  产品声明：本程序为青潮智科产品配套示例程序。
*/

#include "SerialFrame.h"

// 0x02 表示选择歌曲，数据 0x01 和 0x02 分别代表两首歌
#define CMD_SONG 0x02
#define CMD_ERROR 0x7F

// 无源蜂鸣器的驱动输入连接 GPIO 48
#define PIN_BUZZER 48

// ---------------- 串口测试指令 ----------------
// 串口工具设置为：115200 波特率、HEX 发送、HEX 显示
//
// 播放《两只老虎》：
// 发送：55 05 02 01 08 FF
// 应答：55 05 82 01 88 FF
//
// 播放《小星星》：
// 发送：55 05 02 02 09 FF
// 应答：55 05 82 02 89 FF
//
// 测试错误的歌曲编号：
// 发送：55 05 02 03 0A FF
// 应答：55 05 7F 02 86 FF

// 《两只老虎》的音高数组
const int tigerMelody[] = {
  262, 294, 330, 262,
  262, 294, 330, 262,
  330, 349, 392,
  330, 349, 392,
  392, 440, 392, 349, 330, 262,
  392, 440, 392, 349, 330, 262,
  262, 196, 262,
  262, 196, 262
};

// 《两只老虎》中每个音符的持续时间
const int tigerTime[] = {
  400, 400, 400, 400,
  400, 400, 400, 400,
  400, 400, 800,
  400, 400, 800,
  200, 200, 200, 200, 400, 400,
  200, 200, 200, 200, 400, 400,
  400, 400, 800,
  400, 400, 800
};

// 《小星星》的音高数组
const int starMelody[] = {
  262, 262, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 262,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  262, 262, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 262
};

// 《小星星》中每个音符的持续时间
const int starTime[] = {
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800,
  400, 400, 400, 400, 400, 400, 800
};

const int TIGER_COUNT = sizeof(tigerMelody) / sizeof(tigerMelody[0]);
const int STAR_COUNT = sizeof(starMelody) / sizeof(starMelody[0]);

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUZZER, OUTPUT);
}

void loop() {
  if (readFrame()) {
    byte command = getFrameCommand();
    byte data = getFrameData();

    if (command != CMD_SONG) {
      sendFrame(CMD_ERROR, 0x01);
    } else if (data == 0x01) {
      sendFrame(0x82, data);
      playSong(tigerMelody, tigerTime, TIGER_COUNT);
    } else if (data == 0x02) {
      sendFrame(0x82, data);
      playSong(starMelody, starTime, STAR_COUNT);
    } else {
      sendFrame(CMD_ERROR, 0x02);
    }
  }
}

void playSong(const int melody[], const int noteTime[], int noteCount) {
  for (int i = 0; i < noteCount; i = i + 1) {
    tone(PIN_BUZZER, melody[i], noteTime[i]);
    delay(noteTime[i] + 60);
    noTone(PIN_BUZZER);
  }
}
