/*
 * SW3538 串口日志版本
 * 当OLED不工作时，通过串口显示所有信息
 * 提供完整的SW3538数据监控功能
 */

#include <Arduino.h>
#include <Wire.h>
#include "SW3538.h"
#include "global_data.h"

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
    
    // 等待串口连接
    unsigned long startTime = millis();
    while (!Serial && (millis() - startTime < 5000)) {
        ; // 等待串口连接，最多5秒
    }
    
    Serial.println();
    Serial.println("SW3538串口监控系统启动");
    Serial.println("===================");
    Serial.println();
    
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
    
    // 硬件I2C信息
    Serial.println("硬件配置:");
    Serial.println("SW3538 SDA: GPIO8");
    Serial.println("SW3538 SCL: GPIO9");
    Serial.println("I2C地址: 0x3C");
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
            displaySystemInfo();
        } else {
            Serial.println("初始数据读取失败");
        }
    } else {
        Serial.println("SW3538通信失败");
        Serial.println("可能原因:");
        Serial.println("- 接线错误");
        Serial.println("- 电源问题");
        Serial.println("- I2C地址不匹配");
        Serial.println("- SW3538芯片故障");
    }
    
    Serial.println();
    Serial.println("系统启动完成！正在监控数据...");
    Serial.println("================================");
    Serial.println();
    
    // 显示使用说明
    Serial.println("使用说明:");
    Serial.println("- 数据每秒自动更新");
    Serial.println("- 发送 'info' 查看系统信息");
    Serial.println("- 发送 'raw' 查看原始数据");
    Serial.println();
}

void loop() {
    // 非阻塞方式读取和显示数据
    if (getNonBlockingDelay(lastUpdateTime, UPDATE_INTERVAL)) {
        lastUpdateTime = millis();
        
        if (sw3538.readAllData()) {
            sw3538Data = sw3538.data;
            displaySerialData();
        } else {
            Serial.println("[ERROR] 数据读取失败");
        }
    }
    
    // 处理串口命令
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "info") {
            displaySystemInfo();
        } else if (command == "raw") {
            // 显示原始寄存器数据
            Serial.println("原始数据:");
            Serial.print("输入电压: ");
            Serial.print(sw3538Data.inputVoltagemV);
            Serial.println("mV");
            
            Serial.print("输出电压: ");
            Serial.print(sw3538Data.outputVoltagemV);
            Serial.println("mV");
            
            Serial.print("电流1: ");
            Serial.print(sw3538Data.currentPath1mA);
            Serial.println("mA");
            
            Serial.print("电流2: ");
            Serial.print(sw3538Data.currentPath2mA);
            Serial.println("mA");
            
            Serial.print("芯片最大功率: ");
            Serial.print(sw3538Data.maxPowerW);
            Serial.println("W");
            
            Serial.print("快充协议: ");
            Serial.println(sw3538.getFastChargeProtocolString(sw3538Data.fastChargeProtocol));
            
            Serial.print("状态: ");
            Serial.print(sw3538Data.path1Online ? "P1_ON " : "P1_OFF ");
            Serial.print(sw3538Data.path2Online ? "P2_ON " : "P2_OFF ");
            Serial.print(sw3538Data.path1BuckStatus ? "B1_ON " : "B1_OFF ");
            Serial.println(sw3538Data.path2BuckStatus ? "B2_ON" : "B2_OFF");
        }
    }
}

// 显示串口数据
void displaySerialData() {
    
    // 计算功率
    float inputVoltage = sw3538Data.inputVoltagemV / 1000.0;
    float outputVoltage = sw3538Data.outputVoltagemV / 1000.0;
    float current1 = sw3538Data.currentPath1mA / 1000.0;
    float current2 = sw3538Data.currentPath2mA / 1000.0;
    float totalCurrent = current1 + current2;
    float power = outputVoltage * totalCurrent;
    
    
    // 获取协议字符串
    String protocol = sw3538.getFastChargeProtocolString(sw3538Data.fastChargeProtocol);
    
    // 简洁输出
    Serial.println("SW3538监控数据:");
    Serial.print("输入电压: ");
    Serial.print(inputVoltage, 2);
    Serial.println("V");
    
    Serial.print("输出电压: ");
    Serial.print(outputVoltage, 2);
    Serial.println("V");
    
    Serial.print("电流1: ");
    Serial.print(current1, 3);
    Serial.println("A");
    
    Serial.print("电流2: ");
    Serial.print(current2, 3);
    Serial.println("A");
    
    Serial.print("当前功率: ");
    Serial.print(power, 2);
    Serial.println("W");
    
    Serial.print("芯片最大功率: ");
    Serial.print(sw3538Data.maxPowerW);
    Serial.println("W");
    
    Serial.print("快充协议: ");
    Serial.println(protocol);
    
    Serial.print("状态: ");
    Serial.print(sw3538Data.path1Online ? "P1" : "--");
    Serial.print(sw3538Data.path2Online ? " P2" : " --");
    Serial.print(sw3538Data.path1BuckStatus ? " B1" : " --");
    Serial.print(sw3538Data.path2BuckStatus ? " B2" : " --");
    Serial.println();
    Serial.println();
}

// 显示系统信息
void displaySystemInfo() {
    Serial.println();
    Serial.println("系统信息:");
    Serial.print("运行时间: ");
    Serial.print(millis() / 1000);
    Serial.println(" 秒");
    
    Serial.print("可用内存: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    
    Serial.print("CPU频率: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    
    Serial.print("SW3538状态: ");
    Serial.println(sw3538.testI2CAddress(0x3C) ? "正常" : "异常");
    Serial.println();
}

// 非阻塞延迟检查函数
unsigned long getNonBlockingDelay(unsigned long lastTime, unsigned long interval) {
    return (millis() - lastTime >= interval);
}