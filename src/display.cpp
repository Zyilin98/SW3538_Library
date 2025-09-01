/*
 * display.cpp - 精简版OLED显示驱动
 * 
 * 改写说明：
 * 1. 完全移除String类使用
 * 2. 使用char数组和snprintf进行格式化
 * 3. 优化内存使用，减少栈消耗
 * 4. 保持原有显示效果不变
 */

#include "display.h"
#include "SW3538.h"
#include "global_data.h"

// 初始化OLED实例
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 3, /* data=*/ 4, /* reset=*/ U8X8_PIN_NONE);

// OLED显示变量
static int yPosition = 10;
const int Y_INCREMENT = 10;

// 防烧屏功能变量
static bool oledStatus = true;
static unsigned long lastAccessTime = 0;
const unsigned long SCREEN_OFF_TIMEOUT = 30000;
static bool lastPath1Online = false;
static bool lastPath2Online = false;

// 初始化OLED
void initOled() {
    u8g2.begin();
    initButton();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(0, 10, "Initializing...");
    u8g2.sendBuffer();
}

// 显示SW3538数据 - 使用预计算的全局显示数据
void displaySw3538Data() {
    // 实时获取最新的通路状态和buck状态
    bool path1Online = sw3538Data.path1Online;
    bool path2Online = sw3538Data.path2Online;
    bool path1BuckStatus = sw3538Data.path1BuckStatus;
    bool path2BuckStatus = sw3538Data.path2BuckStatus;
    
    if (oledStatus) {
        u8g2.clearBuffer();
        u8g2.drawLine(0, 32, 127, 32);  // 中线
        
        char buf[16];
        
        // 第一通路显示
        u8g2.setFont(u8g2_font_helvR08_tr);
        if (path1Online) u8g2.drawStr(2, 18, "L");
        if (path1BuckStatus) u8g2.drawStr(2, 30, "B");
        
        // 快充状态
        u8g2.setFont(u8g2_font_heisans_tr);
        if (sw3538Data.fastChargeStatus) {
            u8g2.drawStr(12, 30, "Fast");
        } else {
            u8g2.drawStr(12, 30, "  ");
        }
        
        // 第一通路功率
        u8g2.setFont(u8g2_font_helvR14_tr);
        float power1 = displayData.current1 * displayData.outputVoltage;
        snprintf(buf, sizeof(buf), "%.1fW", power1);
        int width = u8g2.getStrWidth(buf);
        u8g2.drawStr(88 - width, 24, buf);
        
        // 第一通路电压电流
        u8g2.setFont(u8g2_font_helvR08_tr);
        snprintf(buf, sizeof(buf), "%.2fV", displayData.outputVoltage);
        width = u8g2.getStrWidth(buf);
        u8g2.drawStr(126 - width, 18, buf);
        
        snprintf(buf, sizeof(buf), "%.2fA", displayData.current1);
        width = u8g2.getStrWidth(buf);
        u8g2.drawStr(126 - width, 30, buf);
        
        // 第二通路显示
        u8g2.setFont(u8g2_font_helvR08_tr);
        if (path2Online) u8g2.drawStr(2, 50, "L");
        if (path2BuckStatus) u8g2.drawStr(2, 62, "B");
        
        // 快充协议
        u8g2.setFont(u8g2_font_heisans_tr);
        u8g2.drawStr(12, 62, SW3538::getProtocolName(sw3538Data.fastChargeProtocol));
        
        // 第二通路功率
        u8g2.setFont(u8g2_font_helvR14_tr);
        float power2 = displayData.current2 * displayData.outputVoltage;
        snprintf(buf, sizeof(buf), "%.1fW", power2);
        width = u8g2.getStrWidth(buf);
        u8g2.drawStr(88 - width, 56, buf);
        
        // 第二通路电压电流
        u8g2.setFont(u8g2_font_helvR08_tr);
        snprintf(buf, sizeof(buf), "%.2fV", displayData.outputVoltage);
        width = u8g2.getStrWidth(buf);
        u8g2.drawStr(126 - width, 50, buf);
        
        snprintf(buf, sizeof(buf), "%.2fA", displayData.current2);
        width = u8g2.getStrWidth(buf);
        u8g2.drawStr(126 - width, 62, buf);
        
        u8g2.sendBuffer();
    }
}

// OLED控制函数 - 保持简单
void turnOnOled() {
    if (!oledStatus) {
        u8g2.setPowerSave(0);
        oledStatus = true;
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        displaySw3538Data();
        Serial.println("[Debug]Turn on the OLED");
    }
}

void turnOffOled() {
    if (oledStatus) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        oledStatus = false;
        Serial.println("[Debug]Turn off the OLED");
    }
}

bool isOledOn() {
    return oledStatus;
}

void updateLastAccessTime() {
    lastAccessTime = millis();
}

void initButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void checkButtonState() {
    static unsigned long lastDebounceTime = 0;      // 上次消抖时间
    static bool lastButtonState = HIGH;             // 上次按钮状态
    static bool buttonPressed = false;              // 按钮已确认按下
    
    bool currentButtonState = digitalRead(BUTTON_PIN);
    unsigned long currentTime = millis();
    
    // 简单高效的消抖算法
    if (currentButtonState != lastButtonState) {
        lastDebounceTime = currentTime;  // 状态变化时重置计时器
    }
    
    // 状态稳定超过40ms才认为是有效状态
    if (currentTime - lastDebounceTime >= 40) {
        if (currentButtonState == LOW && !buttonPressed) {
            // 确认按钮被按下且之前未被确认
            buttonPressed = true;
            updateLastAccessTime();
            if (!isOledOn()) {
                turnOnOled();
                Serial.println("[Debug]Button press-Turn on the OLED");
            }
        } else if (currentButtonState == !LOW) {
            // 按钮释放，重置确认状态
            buttonPressed = false;
        }
    }
    
    lastButtonState = currentButtonState;
}

void checkOledTimeout() {
    if (oledStatus && (millis() - lastAccessTime > SCREEN_OFF_TIMEOUT)) {
        turnOffOled();
        Serial.println("[Debug]Timeout-Turn off the OLED");
    }
}
void pluginCheck() {
    // 实时获取最新的通路状态
    bool currentPath1Online = sw3538Data.path1Online;
    bool currentPath2Online = sw3538Data.path2Online;
    
    // 检查通路状态是否发生变化
    bool pathStatusChanged = (currentPath1Online != lastPath1Online) || 
                            (currentPath2Online != lastPath2Online);
    
    if (pathStatusChanged && !isOledOn()) {
        turnOnOled();
        updateLastAccessTime();
        Serial.println("[Debug]Plugin check-Turn on the OLED");
    }
    
    // 更新上一次的状态记录
    lastPath1Online = currentPath1Online;
    lastPath2Online = currentPath2Online;
}