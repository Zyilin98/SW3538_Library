#include "display.h"

// 初始化OLED实例
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 3, /* data=*/ 4, /* reset=*/ U8X8_PIN_NONE);

// OLED显示变量
static int yPosition = 10; // 初始Y位置
const int Y_INCREMENT = 10; // Y方向移动增量

// 防烧屏功能变量
static bool oledStatus = true; // OLED当前状态，默认为开启
static unsigned long lastAccessTime = 0; // 最后一次操作时间（通路状态变化或串口命令）
const unsigned long SCREEN_OFF_TIMEOUT = 30000; // 30秒无操作后关闭屏幕
// 跟踪上一次通路状态，用于检测状态变化
static bool lastPath1Online = false;
static bool lastPath2Online = false;

// 初始化OLED
void initOled() {
    u8g2.begin();
    initButton(); // 初始化按钮
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.drawStr(0, 10, "Initializing...");
    u8g2.sendBuffer();
}

// 显示SW3538数据
void displaySw3538Data(float inputVoltage, float outputVoltage, float current1, float current2, float power, bool path1Online, bool path2Online, bool path1BuckStatus, bool path2BuckStatus) {
    // 检查通路状态是否发生变化（接入或断开）
    bool pathStatusChanged = (path1Online != lastPath1Online) || (path2Online != lastPath2Online);
    
    // 如果通路状态变化（接入或断开），则更新最后操作时间并打开OLED
    if (pathStatusChanged) {
        updateLastAccessTime();
        if (!isOledOn()) {
            turnOnOled();
        }
    }
    
    // 更新上一次通路状态
    lastPath1Online = path1Online;
    lastPath2Online = path2Online;
    
    // 如果OLED开启，则执行显示操作
    if (oledStatus) {
        u8g2.clearBuffer();
        
        // 在Y轴中线位置绘制水平线（128x64屏幕，中线Y坐标为32）
        u8g2.drawLine(0, 32, 127, 32);

        // 设置右侧对齐的基准X坐标，增加到128使电流的'A'紧贴屏幕右侧边框
        int rightAlignX = 126;  // 右侧内容起始X坐标(屏幕宽度为128像素)

        // 线上区域（第一通路）
        // 左侧显示L和B（较小字号，不加粗）
        u8g2.setFont(u8g2_font_helvR08_tr);  // 较小字号
        if (path1Online) {
            u8g2.drawStr(10, 18, "L");
        }
        if (path1BuckStatus) {
            u8g2.drawStr(10, 30, "B");
        }

        // 显示通路一功率（增大字号并加粗）
        u8g2.setFont(u8g2_font_helvR14_tr);  // 较大字号
        char power1Str[10];
        // 计算通路一功率（假设功率参数是总功率，这里需要根据实际情况调整）
        float power1 = current1 * outputVoltage;
        snprintf(power1Str, sizeof(power1Str), "%.1fW", power1);
        int power1Width = u8g2.getStrWidth(power1Str);
        // 功率显示在左侧L/B和右侧参数之间
        // 功率显示右侧对齐到X=86的位置
        u8g2.drawStr(88 - power1Width, 24, power1Str);

        // 右侧显示电压和电流（靠右对齐）
        u8g2.setFont(u8g2_font_helvR08_tr);  // 较小字号
        char voltage1Str[10];
        snprintf(voltage1Str, sizeof(voltage1Str), "%.2fV", outputVoltage);
        int voltage1Width = u8g2.getStrWidth(voltage1Str);
        u8g2.drawStr(rightAlignX - voltage1Width, 18, voltage1Str);

        char current1Str[10];
        snprintf(current1Str, sizeof(current1Str), "%.2fA", current1);
        int current1Width = u8g2.getStrWidth(current1Str);
        u8g2.drawStr(rightAlignX - current1Width, 30, current1Str);

        // 线下区域（第二通路）
        // 左侧显示L和B（较小字号，不加粗）
        u8g2.setFont(u8g2_font_helvR08_tr);
        if (path2Online) {
            u8g2.drawStr(10, 50, "L");
        }
        if (path2BuckStatus) {
            u8g2.drawStr(10, 62, "B");
        }

        // 显示通路二功率（增大字号并加粗）
        u8g2.setFont(u8g2_font_helvR14_tr);
        char power2Str[10];
        // 计算通路二功率
        float power2 = current2 * outputVoltage;
        snprintf(power2Str, sizeof(power2Str), "%.1fW", power2);
        int power2Width = u8g2.getStrWidth(power2Str);
        // 功率显示右侧对齐到X=88的位置
        u8g2.drawStr(88 - power2Width, 56, power2Str);

        // 右侧显示电压和电流（靠右对齐）
        u8g2.setFont(u8g2_font_helvR08_tr);
        char voltage2Str[10];
        snprintf(voltage2Str, sizeof(voltage2Str), "%.2fV", outputVoltage);
        int voltage2Width = u8g2.getStrWidth(voltage2Str);
        u8g2.drawStr(rightAlignX - voltage2Width, 50, voltage2Str);

        // 右侧显示电流（靠右对齐）
        char current2Str[10];
        snprintf(current2Str, sizeof(current2Str), "%.2fA", current2);
        int current2Width = u8g2.getStrWidth(current2Str);
        u8g2.drawStr(rightAlignX - current2Width, 62, current2Str);


        /*
        // 显示输入电压（最小字号，右下角）
        char inputVoltageStr[10];
        snprintf(inputVoltageStr, sizeof(inputVoltageStr), "Vin: %.1fV", inputVoltage);
        int strWidth = u8g2.getStrWidth(inputVoltageStr);
        u8g2.drawStr(128 - strWidth, 60, inputVoltageStr);
        */
        u8g2.sendBuffer();
    }
    
    // 检查是否需要关闭OLED（防烧屏）
    checkOledTimeout();
}

// 打开OLED
void turnOnOled() {
    if (!oledStatus) {
        u8g2.setPowerSave(0); // 关闭省电模式
        oledStatus = true;
        u8g2.clearBuffer();
        u8g2.sendBuffer();
    }
}

// 关闭OLED
void turnOffOled() {
    if (oledStatus) {
        u8g2.clearBuffer();
        u8g2.sendBuffer();
        u8g2.setPowerSave(1); // 开启省电模式
        oledStatus = false;
    }
}

// 检查OLED是否开启
bool isOledOn() {
    return oledStatus;
}

// 更新通路接入时间
void updateLastAccessTime() {
    lastAccessTime = millis();
}

// 初始化按钮
void initButton() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // 设置按钮引脚为输入模式，启用内部上拉电阻
}

// 检查按钮状态
void checkButtonState() {
    // 读取按钮状态（低电平表示按下）
    if (digitalRead(BUTTON_PIN) == LOW) {
        // 按钮被按下，点亮屏幕并更新最后操作时间
        updateLastAccessTime();
        if (!isOledOn()) {
            turnOnOled();
        }
        // 简单的防抖动
        delay(100);
    }
}

// 检查并更新OLED状态（防烧屏）
void checkOledTimeout() {
    if (oledStatus && (millis() - lastAccessTime > SCREEN_OFF_TIMEOUT)) {
        turnOffOled();
    }
}

// OLED显示Hello World并下移（保留用于测试）
void displayOledHelloWorld() {
    if (oledStatus) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_helvR08_tr);
        u8g2.drawStr(0, yPosition, "Hello world");
        u8g2.sendBuffer();
        Serial.print("[Debug]Hello world is down");
        // 更新Y位置，到达底部后重置
        yPosition += Y_INCREMENT;
        if (yPosition > 56) { // 128x64 OLED，留出8px底部边距
            yPosition = 10;
        }
    }
}