/*
 * 示例：如何使用全局SW3538数据
 * 
 * 这个示例展示了如何在其他函数中访问SW3538数据
 */

#include "global_data.h"
#include <Arduino.h>

// 示例函数：获取当前功率
float getCurrentPower() {
    const SW3538_Data_t& data = getSW3538Data();
    if (!isSW3538DataValid()) {
        return 0.0;
    }
    
    float power1 = (data.outputVoltagemV / 1000.0) * (data.currentPath1mA / 1000.0);
    float power2 = (data.outputVoltagemV / 1000.0) * (data.currentPath2mA / 1000.0);
    return power1 + power2;
}

// 示例函数：检查是否有设备连接
bool isDeviceConnected(int path) {
    const SW3538_Data_t& data = getSW3538Data();
    
    if (path == 1) {
        return data.path1Online;
    } else if (path == 2) {
        return data.path2Online;
    }
    return false;
}

// 示例函数：获取电压信息
String getVoltageInfo() {
    const SW3538_Data_t& data = getSW3538Data();
    
    if (!isSW3538DataValid()) {
        return "No data";
    }
    
    float inputV = data.inputVoltagemV / 1000.0;
    float outputV = data.outputVoltagemV / 1000.0;
    
    return "Input: " + String(inputV, 2) + "V, Output: " + String(outputV, 2) + "V";
}

// 示例函数：获取充电协议字符串
String getProtocolString() {
    const SW3538_Data_t& data = getSW3538Data();
    
    switch (data.fastChargeProtocol) {
        case SW3538_FC_NONE: return "None";
        case SW3538_FC_QC2_0: return "QC2.0";
        case SW3538_FC_QC3_0: return "QC3.0";
        case SW3538_FC_QC3_PLUS: return "QC3+";
        case SW3538_FC_FCP: return "FCP";
        case SW3538_FC_SCP: return "SCP";
        case SW3538_FC_PD_FIX: return "PD";
        case SW3538_FC_PD_PPS: return "PD PPS";
        default: return "Unknown";
    }
}

// 示例函数：获取状态字符串
String getStatusString() {
    const SW3538_Data_t& data = getSW3538Data();
    
    String status = "";
    
    if (data.path1Online) status += "P1L ";
    if (data.path1BuckStatus) status += "P1B ";
    if (data.path2Online) status += "P2L ";
    if (data.path2BuckStatus) status += "P2B ";
    
    if (status.length() == 0) {
        status = "No devices";
    }
    
    return status;
}

// 示例函数：计算效率
float calculateEfficiency() {
    const SW3538_Data_t& data = getSW3538Data();
    
    if (!isSW3538DataValid()) {
        return 0.0;
    }
    
    float inputPower = (data.inputVoltagemV / 1000.0) * 
                      ((data.currentPath1mA + data.currentPath2mA) / 1000.0);
    float outputPower = getCurrentPower();
    
    if (inputPower > 0) {
        return (outputPower / inputPower) * 100.0;
    }
    return 0.0;
}

// 示例用法
void setup() {
    Serial.begin(115200);
    
    Serial.println("=== SW3538数据访问示例 ===");
    
    // 注意：这个示例需要与主程序一起使用
    // 主程序会负责更新sw3538Data变量
}

void loop() {
    // 每5秒打印一次信息
    static unsigned long lastPrint = 0;
    
    if (millis() - lastPrint > 5000) {
        Serial.println("=== 当前SW3538数据 ===");
        Serial.println("功率: " + String(getCurrentPower(), 2) + "W");
        Serial.println("电压: " + getVoltageInfo());
        Serial.println("协议: " + getProtocolString());
        Serial.println("状态: " + getStatusString());
        Serial.println("效率: " + String(calculateEfficiency(), 1) + "%");
        Serial.println("设备1连接: " + String(isDeviceConnected(1) ? "是" : "否"));
        Serial.println("设备2连接: " + String(isDeviceConnected(2) ? "是" : "否"));
        Serial.println("====================");
        
        lastPrint = millis();
    }
}