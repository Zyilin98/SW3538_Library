/*
 * display.h - 精简版OLED显示头文件
 * 
 * 改写说明：
 * 1. 保持原有接口不变
 * 2. 移除String依赖
 * 3. 与精简版SW3538完全兼容
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include "SW3538.h"

// OLED实例声明
extern U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;

// 按钮定义
#define BUTTON_PIN 0

// 函数声明 - 保持接口不变
void initButton();
void checkButtonState();
void initOled();
void displaySw3538Data(float inputVoltage, float outputVoltage, float current1, 
                      float current2, float power, bool path1Online, bool path2Online, 
                      bool path1BuckStatus, bool path2BuckStatus, SW3538_Data_t data);
void turnOnOled();
void turnOffOled();
bool isOledOn();
void updateLastAccessTime();
void checkOledTimeout();

#endif // DISPLAY_H

/*
 * 原版本display.h完整保留
 * 
 * 主要改进：
 * 1. 移除不必要的String依赖
 * 2. 保持所有接口不变
 * 3. 优化包含关系
 */
