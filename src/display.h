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
/**
 * @brief 显示SW3538数据
 * 
 * 使用预计算的全局显示数据刷新OLED显示
 * 显示内容包括电压、电流、功率、快充状态等信息
 */
void displaySw3538Data();
void turnOnOled();
void turnOffOled();
bool isOledOn();
void updateLastAccessTime();
void checkOledTimeout();
void pluginCheck();
#endif // DISPLAY_H
