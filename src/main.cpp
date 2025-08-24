#include <Arduino.h>
#include <Wire.h>
#include "SW3538.h"
#include "global_data.h"
#include "display.h"

// SW3538实例
SW3538 sw3538;

// 非阻塞定时变量
const unsigned long UPDATE_INTERVAL = 1000; // 1秒更新间隔
unsigned long lastUpdateTime = 0;

// 函数声明
void displaySerialData();
void displaySystemInfo();
unsigned long getNonBlockingDelay(unsigned long lastTime, unsigned long interval);

void setup() {
    Serial.begin(115200);
    
    // 初始化OLED
    initOled();
    // 初始化防烧屏功能
    updateLastAccessTime(); // 设置初始访问时间
    
    // 系统信息
    Serial.println("系统信息:");
    Serial.print("MCU: ESP32-C3");
    Serial.print(" 时钟频率: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println("MHz");
    Serial.print("可用内存: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println();
    
    // 初始化SW3538
    Serial.println("初始化SW3538...");
    sw3538.begin();
    
    // 测试通信
    Serial.println("测试SW3538通信...");
    if (sw3538.testI2CAddress(0x3C)) {
        Serial.println("SW3538通信正常");
        
        // 读取初始数据
        if (sw3538.readAllData()) {
            Serial.println("初始数据读取成功");
        } else {
            Serial.println("初始数据读取失败");
        }
    }

}

void loop() {
    // 检查按钮状态
    checkButtonState();
    // 非阻塞方式读取和显示数据
    if (getNonBlockingDelay(lastUpdateTime, UPDATE_INTERVAL)) {
        lastUpdateTime = millis();
        
        // 读取SW3538数据
        if (sw3538.readAllData()) {
            // 调用SW3538驱动内的串口日志打印功能
            sw3538.printAllData(Serial);
            
            // 计算显示所需数据
            sw3538Data = sw3538.data;
            float inputVoltage = sw3538Data.inputVoltagemV / 1000.0;
            float outputVoltage = sw3538Data.outputVoltagemV / 1000.0;
            float current1 = sw3538Data.currentPath1mA / 1000.0;
            float current2 = sw3538Data.currentPath2mA / 1000.0;
            float totalCurrent = current1 + current2;
            float power = outputVoltage * totalCurrent;
            
            // 更新OLED显示
            displaySw3538Data(inputVoltage, outputVoltage, current1, current2, power,
                             sw3538Data.path1Online, sw3538Data.path2Online,
                             sw3538Data.path1BuckStatus, sw3538Data.path2BuckStatus,
                             sw3538Data);
        } else {
            Serial.println("[ERROR] 数据读取失败");
        }
    }
}

// 非阻塞延迟函数
unsigned long getNonBlockingDelay(unsigned long lastTime, unsigned long interval) {
    unsigned long currentTime = millis();
    if (currentTime - lastTime >= interval) {
        return currentTime;
    }
    return 0;
}