#include <Arduino.h>
#include <Wire.h>
#include "SW3538.h"
#include "global_data.h"
#include "display.h"
#include "adaptive_scan.h"

// SW3538实例
SW3538 sw3538;
AdaptiveScan aScan;

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
            // 初始化显示数据
            sw3538Data = sw3538.data;
            updateDisplayData(sw3538Data);
        } else {
            Serial.println("初始数据读取失败");
        }
    }
    // 初始化自适应扫描
    aScan.begin();
    aScan.setEpsilon(30);
}

void loop() {
    // 检查按钮状态
    checkButtonState();
    checkOledTimeout();
    // 自适应方式读取和显示数据
    if (aScan.tick()){
        
        // 读取SW3538数据
        if (sw3538.readAllData()) {
            // 调用SW3538驱动内的串口日志打印功能
            sw3538.printAllData(Serial);
            float total_ma = sw3538.data.currentPath1mA +
                             sw3538.data.currentPath2mA;
            aScan.updateCurrent(total_ma); // 自适应调周期
            
            // 更新全局数据
            sw3538Data = sw3538.data;
            
            // 计算并存储显示数据
            updateDisplayData(sw3538Data);
            
            // 更新OLED显示
            displaySw3538Data();
        } else {
            Serial.println("[ERROR] 数据读取失败");
        }
    }
}
