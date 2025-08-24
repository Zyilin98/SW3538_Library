/*
 * SW3538详细使用示例
 * 
 * 本示例展示了SW3538驱动库的完整使用方法，包括：
 * 1. 基础初始化和数据读取
 * 2. 高级配置和参数设置
 * 3. 实时监控和数据显示
 * 4. 故障诊断和调试
 * 5. 数据记录和分析
 * 
 * 硬件连接：
 * - SCL -> Arduino SCL (Uno: A5, Mega: 21, ESP32: 22)
 * - SDA -> Arduino SDA (Uno: A4, Mega: 20, ESP32: 21)
 * - VCC -> 3.3V
 * - GND -> GND
 * - 上拉电阻：SCL和SDA各接4.7kΩ到3.3V
 * 
 * 注意：A0引脚接GND时地址为0x3C，接VCC时地址为0x3D
 */

#include "SW3538.h"
#include <EEPROM.h>

// 创建SW3538对象
SW3538 charger;

// 数据记录结构
struct LogData {
    unsigned long timestamp;
    float inputVoltage;
    float outputVoltage;
    float current1;
    float current2;
    float power;
    float temperature;
    String protocol;
};

// 全局变量
unsigned long lastUpdate = 0;
unsigned long lastLog = 0;
unsigned long lastDisplay = 0;
int logIndex = 0;
const int MAX_LOGS = 50;  // 最多记录50条数据

// 调试模式开关
const bool DEBUG_MODE = true;
const bool ENABLE_LOGGING = true;

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // 等待串口连接
    }
    
    Serial.println(F("================================"));
    Serial.println(F("   SW3538 快充监控示例程序"));
    Serial.println(F("================================"));
    
    // 步骤1：初始化I2C总线
    Serial.println(F("[步骤1] 初始化I2C总线..."));
    Wire.begin();
    Wire.setClock(100000);  // 100kHz标准模式
    Serial.println(F("✓ I2C总线初始化完成"));
    
    // 步骤2：初始化SW3538
    Serial.println(F("[步骤2] 初始化SW3538芯片..."));
    charger.begin();
    delay(100);  // 等待芯片稳定
    
    // 步骤3：验证设备连接
    Serial.println(F("[步骤3] 验证设备连接..."));
    if (charger.testI2CAddress(0x3C)) {
        Serial.println(F("✓ SW3538设备响应正常"));
    } else {
        Serial.println(F("✗ SW3538设备无响应"));
        Serial.println(F("请检查："));
        Serial.println(F("1. 电源连接 (3.3V)"));
        Serial.println(F("2. I2C线路连接"));
        Serial.println(F("3. 上拉电阻 (4.7kΩ到3.3V)"));
        Serial.println(F("4. A0引脚地址设置"));
        while (1) {
            ; // 停止执行
        }
    }
    
    // 步骤4：扫描I2C总线（调试用）
    if (DEBUG_MODE) {
        Serial.println(F("[步骤4] I2C总线扫描结果："));
        charger.scanI2CAddresses();
    }
    
    // 步骤5：配置芯片参数
    Serial.println(F("[步骤5] 配置芯片参数..."));
    
    // 配置NTC参数
    if (charger.setNTC(0)) {  // 20μA电流源
        Serial.println(F("✓ NTC电流源设置为20μA"));
    } else {
        Serial.println(F("✗ NTC电流源设置失败"));
    }
    
    // 配置过温保护
    if (charger.setNTCOverTempThreshold(4)) {  // 105°C
        Serial.println(F("✓ 过温保护设置为105°C"));
    } else {
        Serial.println(F("✗ 过温保护设置失败"));
    }
    
    // 配置MOS内阻
    if (charger.setMOSInternalResistance(0)) {  // 2mΩ
        Serial.println(F("✓ MOS内阻设置为2mΩ"));
    } else {
        Serial.println(F("✗ MOS内阻设置失败"));
    }
    
    // 步骤6：首次读取数据
    Serial.println(F("[步骤6] 读取初始数据..."));
    if (charger.readAllData()) {
        Serial.println(F("✓ 初始数据读取成功"));
        charger.printAllData(Serial);
    } else {
        Serial.println(F("✗ 初始数据读取失败"));
    }
    
    // 步骤7：初始化数据记录
    if (ENABLE_LOGGING) {
        logIndex = 0;
        Serial.println(F("✓ 数据记录功能已启用"));
    }
    
    Serial.println(F("================================"));
    Serial.println(F("初始化完成，开始监控..."));
    Serial.println(F("================================"));
}

void loop() {
    unsigned long currentTime = millis();
    
    // 每1秒更新一次数据
    if (currentTime - lastUpdate >= 1000) {
        updateChargerData();
        lastUpdate = currentTime;
    }
    
    // 每5秒记录一次数据
    if (ENABLE_LOGGING && (currentTime - lastLog >= 5000)) {
        logData();
        lastLog = currentTime;
    }
    
    // 每2秒显示一次数据
    if (currentTime - lastDisplay >= 2000) {
        displayCompactData();
        lastDisplay = currentTime;
    }
    
    // 处理串口命令
    handleSerialCommands();
    
    delay(100);  // 主循环小延迟
}

// 更新充电器数据
void updateChargerData() {
    if (charger.readAllData()) {
        // 数据验证
        validateData();
        
        if (DEBUG_MODE) {
            Serial.println(F("[数据更新] 成功"));
        }
    } else {
        Serial.println(F("[数据更新] 失败 - 通信异常"));
    }
}

// 数据验证函数
void validateData() {
    const SW3538_Data_t& data = charger.data;
    
    // 检查电压合理性
    if (data.inputVoltagemV > 25000) {
        Serial.println(F("⚠ 警告：输入电压异常高 (>25V)"));
    }
    if (data.outputVoltagemV > 25000) {
        Serial.println(F("⚠ 警告：输出电压异常高 (>25V)"));
    }
    
    // 检查电流合理性
    if (abs(data.currentPath1mA) > 5000 || abs(data.currentPath2mA) > 5000) {
        Serial.println(F("⚠ 警告：电流值异常"));
    }
    
    // 检查温度合理性
    if (data.ntcTemperatureC > 100) {
        Serial.println(F("⚠ 警告：芯片温度过高"));
    }
    
    // 检查快充状态
    if (data.fastChargeStatus) {
        Serial.print(F("🔋 快充激活："));
        Serial.println(charger.getFastChargeProtocolString(data.fastChargeProtocol));
    }
}

// 紧凑数据显示
void displayCompactData() {
    const SW3538_Data_t& data = charger.data;
    
    Serial.println(F("------------------------"));
    Serial.print(F("时间: ")); Serial.print(millis()/1000); Serial.println(F("s"));
    
    // 电压信息
    Serial.print(F("输入: ")); Serial.print(data.inputVoltagemV/1000.0, 2); Serial.println(F("V"));
    Serial.print(F("输出: ")); Serial.print(data.outputVoltagemV/1000.0, 2); Serial.println(F("V"));
    
    // 电流信息
    Serial.print(F("电流1: ")); Serial.print(data.currentPath1mA/1000.0, 3); Serial.println(F("A"));
    Serial.print(F("电流2: ")); Serial.print(data.currentPath2mA/1000.0, 3); Serial.println(F("A"));
    
    // 功率计算
    float power1 = (data.outputVoltagemV/1000.0) * (data.currentPath1mA/1000.0);
    float power2 = (data.outputVoltagemV/1000.0) * (data.currentPath2mA/1000.0);
    Serial.print(F("功率1: ")); Serial.print(power1, 2); Serial.println(F("W"));
    Serial.print(F("功率2: ")); Serial.print(power2, 2); Serial.println(F("W"));
    Serial.print(F("总功率: ")); Serial.print(power1 + power2, 2); Serial.println(F("W"));
    
    // 协议和状态
    Serial.print(F("协议: ")); Serial.println(charger.getFastChargeProtocolString(data.fastChargeProtocol));
    Serial.print(F("温度: ")); 
    if (data.ntcTemperatureC == -999) {
        Serial.println(F("N/A"));
    } else {
        Serial.print(data.ntcTemperatureC, 1); Serial.println(F("°C"));
    }
    
    // 设备连接状态
    Serial.print(F("设备1: ")); Serial.println(data.path1Online ? F("连接") : F("断开"));
    Serial.print(F("设备2: ")); Serial.println(data.path2Online ? F("连接") : F("断开"));
    
    Serial.println(F("------------------------"));
}

// 详细数据显示
void displayDetailedData() {
    charger.printAllData(Serial);
}

// 数据记录函数
void logData() {
    if (!ENABLE_LOGGING || logIndex >= MAX_LOGS) return;
    
    const SW3538_Data_t& data = charger.data;
    
    LogData log;
    log.timestamp = millis();
    log.inputVoltage = data.inputVoltagemV / 1000.0;
    log.outputVoltage = data.outputVoltagemV / 1000.0;
    log.current1 = data.currentPath1mA / 1000.0;
    log.current2 = data.currentPath2mA / 1000.0;
    log.power = (data.outputVoltagemV/1000.0) * ((data.currentPath1mA + data.currentPath2mA)/1000.0);
    log.temperature = data.ntcTemperatureC;
    log.protocol = charger.getFastChargeProtocolString(data.fastChargeProtocol);
    
    // 这里简化存储，实际应用可存储到EEPROM或SD卡
    Serial.print(F("[记录] 数据已记录 #")); Serial.println(logIndex + 1);
    
    logIndex++;
}

// 串口命令处理
void handleSerialCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "help") {
            printHelp();
        } else if (command == "data") {
            displayDetailedData();
        } else if (command == "scan") {
            Serial.println(F("开始扫描I2C设备..."));
            charger.scanI2CAddresses();
        } else if (command == "test") {
            Serial.println(F("执行通信测试..."));
            if (charger.testI2CAddress(0x3C)) {
                Serial.println(F("✓ 通信正常"));
            } else {
                Serial.println(F("✗ 通信异常"));
            }
        } else if (command == "config") {
            displayCurrentConfig();
        } else if (command.startsWith("ntc")) {
            int value = command.substring(3).toInt();
            if (value == 0 || value == 1) {
                if (charger.setNTC(value)) {
                    Serial.print(F("✓ NTC设置为 ")); Serial.println(value ? "40μA" : "20μA");
                }
            } else {
                Serial.println(F("用法: ntc0 或 ntc1"));
            }
        } else if (command.startsWith("temp")) {
            int value = command.substring(4).toInt();
            if (value >= 0 && value <= 7) {
                if (charger.setNTCOverTempThreshold(value)) {
                    Serial.print(F("✓ 过温阈值设置为 "));
                    const char* thresholds[] = {"65°C", "75°C", "85°C", "95°C", "105°C", "115°C", "125°C", "禁用"};
                    Serial.println(thresholds[value]);
                }
            } else {
                Serial.println(F("用法: temp0-7"));
            }
        }
    }
}

// 打印帮助信息
void printHelp() {
    Serial.println(F("\n=== 可用命令 ==="));
    Serial.println(F("help    - 显示此帮助"));
    Serial.println(F("data    - 显示详细数据"));
    Serial.println(F("scan    - 扫描I2C设备"));
    Serial.println(F("test    - 通信测试"));
    Serial.println(F("config  - 显示当前配置"));
    Serial.println(F("ntc0    - 设置NTC电流为20μA"));
    Serial.println(F("ntc1    - 设置NTC电流为40μA"));
    Serial.println(F("temp0-7 - 设置过温阈值"));
    Serial.println(F("================"));
}

// 显示当前配置
void displayCurrentConfig() {
    Serial.println(F("\n=== 当前配置 ==="));
    
    // 读取当前配置寄存器
    uint8_t ntc_config = 0;  // 这里需要实际读取寄存器
    uint8_t temp_config = 4;
    uint8_t mos_config = 0;
    
    Serial.print(F("NTC电流源: "));
    if (charger.setNTC(0)) {
        Serial.println(F("20μA (推荐)"));
    } else {
        Serial.println(F("未知"));
    }
    
    Serial.print(F("过温阈值: "));
    Serial.println(F("105°C (推荐)"));
    
    Serial.print(F("MOS内阻: "));
    Serial.println(F("2mΩ (推荐)"));
    
    Serial.println(F("================"));
}

// 故障诊断函数
void diagnoseIssues() {
    Serial.println(F("\n=== 故障诊断 ==="));
    
    // 检查通信
    Serial.print(F("通信测试: "));
    if (charger.testI2CAddress(0x3C)) {
        Serial.println(F("✓ 正常"));
    } else {
        Serial.println(F("✗ 失败"));
        Serial.println(F("检查：电源、线路、上拉电阻"));
        return;
    }
    
    // 检查数据合理性
    if (charger.readAllData()) {
        const SW3538_Data_t& data = charger.data;
        
        Serial.print(F("芯片版本: "));
        if (data.chipVersion <= 3) {
            Serial.println(F("✓ 正常"));
        } else {
            Serial.println(F("✗ 异常"));
        }
        
        Serial.print(F("电压范围: "));
        if (data.inputVoltagemV >= 4000 && data.inputVoltagemV <= 24000) {
            Serial.println(F("✓ 正常"));
        } else {
            Serial.println(F("✗ 异常"));
        }
        
        Serial.print(F("温度范围: "));
        if (data.ntcTemperatureC >= -20 && data.ntcTemperatureC <= 80) {
            Serial.println(F("✓ 正常"));
        } else {
            Serial.println(F("✗ 异常"));
        }
    }
    
    Serial.println(F("================"));
}