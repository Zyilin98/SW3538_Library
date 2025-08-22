#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include "SW3538.h"

// 声明OLED实例为外部变量
extern U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;

// 按钮定义
#define BUTTON_PIN 0  // 按钮连接的GPIO引脚

// 函数声明
void initButton();
void checkButtonState();

void initOled();
void displaySw3538Data(float inputVoltage, float outputVoltage, float current1, float current2, float power, bool path1Online, bool path2Online, bool path1BuckStatus, bool path2BuckStatus, SW3538_Data_t data);
void displayOledHelloWorld(); // 保留原函数，用于测试
void turnOnOled();
void turnOffOled();
bool isOledOn();
void updateLastAccessTime();
void checkOledTimeout();

#endif // DISPLAY_H