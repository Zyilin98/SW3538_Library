/*
 * SW3538.cpp - 精简版驱动实现
 * 
 * 改写说明：
 * 1. 完全移除String类，使用char数组和标准C函数
 * 2. 简化日志系统，移除分级机制
 * 3. 修复ADC读取逻辑和错误处理
 * 4. 优化内存使用，减少栈消耗
 * 
 * 修复的问题：
 * - 修复ADC通道选择逻辑
 * - 修复温度计算精度
 * - 优化I2C通信重试机制
 * - 添加数据有效性检查
 */

#include <Arduino.h>
#include "SW3538.h"
#include <Wire.h>

// 构造函数 - 简化实现
SW3538::SW3538(uint8_t address) : _address(address), _sdaPin(-1), _sclPin(-1), _useCustomPins(false) {
    SW3538_LOG_VAL("SW3538 init addr: 0x", address);
}

// 支持自定义I2C引脚的构造函数
SW3538::SW3538(uint8_t address, int sdaPin, int sclPin) : _address(address), _sdaPin(sdaPin), _sclPin(sclPin), _useCustomPins(true) {
    SW3538_LOG_VAL("SW3538 init addr: 0x", address);
    SW3538_LOG_VAL("SDA pin: ", sdaPin);
    SW3538_LOG_VAL("SCL pin: ", sclPin);
}

// 测试I2C地址 - 使用char数组替代String
bool SW3538::testI2CAddress(uint8_t address) {
    char buf[32];
    snprintf(buf, sizeof(buf), "Testing addr 0x%02X... ", address);
    Serial.print(buf);
    
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
        Serial.print("OK ");
        
        uint8_t testReg = readRegister(SW3538_REG_VERSION);
        if (testReg != 0xFF && testReg != 0x00) {
            Serial.println("SW3538 found");
            return true;
        } else {
            Serial.println("Invalid data");
            return false;
        }
    } else {
        Serial.println("No response");
        return false;
    }
}

// 扫描I2C地址 - 简化输出
void SW3538::scanI2CAddresses() {
    SW3538_LOG("I2C scan start");
    Serial.println("Addr  Status");
    
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "0x%02X  FOUND", addr);
            Serial.println(buf);
            found++;
        }
    }
    
    SW3538_LOG_VAL("Found devices: ", found);
}

// 初始化 - 简化实现
void SW3538::begin() {
    SW3538_LOG("SW3538 init");
    
    // 根据是否使用自定义引脚来初始化I2C
    if (_useCustomPins) {
        // 使用自定义SDA/SCL引脚
        Wire.begin(_sdaPin, _sclPin);
        SW3538_LOG_VAL("I2C started with custom pins - SDA: ", _sdaPin);
        SW3538_LOG_VAL("SCL: ", _sclPin);
    } else {
        // 使用默认引脚
        Wire.begin();
        SW3538_LOG("I2C started with default pins");
    }
    
    Wire.setClock(100000);
    
    uint8_t version = readRegister(SW3538_REG_VERSION);
    if (version == 0xFF || version == 0x00) {
        SW3538_LOG("Communication failed");
    } else {
        SW3538_LOG_VAL("Chip version: ", version);
    }
}

// 读取寄存器 - 优化重试机制
uint8_t SW3538::readRegister(uint16_t reg) {
    uint8_t reg_addr = (uint8_t)(reg & 0xFF);
    
    for (int retry = 0; retry < 3; retry++) {
        Wire.beginTransmission(_address);
        Wire.write(reg_addr);
        
        if (Wire.endTransmission(false) != 0) {
            delay(5 << retry);  // 指数退避
            continue;
        }
        
        if (Wire.requestFrom(_address, (uint8_t)1) == 1) {
            return Wire.read();
        }
        
        delay(5 << retry);
    }
    
    return 0xFF;  // 通信失败
}

// 写入寄存器 - 简化实现
bool SW3538::writeRegister(uint16_t reg, uint8_t value) {
    uint8_t reg_addr = (uint8_t)(reg & 0xFF);
    
    for (int retry = 0; retry < 3; retry++) {
        Wire.beginTransmission(_address);
        Wire.write(reg_addr);
        Wire.write(value);
        
        if (Wire.endTransmission() == 0) {
            return true;
        }
        
        delay(5);
    }
    
    return false;
}

// 启用I2C写操作 - 简化序列
bool SW3538::enableI2CWrite() {
    return writeRegister(SW3538_REG_I2C_ENABLE, 0x20) &&
           writeRegister(SW3538_REG_I2C_ENABLE, 0x40) &&
           writeRegister(SW3538_REG_I2C_ENABLE, 0x80);
}

// 启用强制操作写 - 简化序列
bool SW3538::enableForceOperationWrite() {
    return writeRegister(SW3538_REG_FORCE_OP_ENABLE, 0x20) &&
           writeRegister(SW3538_REG_FORCE_OP_ENABLE, 0x40) &&
           writeRegister(SW3538_REG_FORCE_OP_ENABLE, 0x80);
}

// ADC控制 - 简化实现
bool SW3538::enableADC(uint8_t adc_type) {
    if (!enableForceOperationWrite()) return false;
    
    uint8_t reg_val = readRegister(SW3538_REG_FORCE_OP2);
    reg_val |= (1 << adc_type);
    return writeRegister(SW3538_REG_FORCE_OP2, reg_val);
}

bool SW3538::disableADC(uint8_t adc_type) {
    if (!enableForceOperationWrite()) return false;
    
    uint8_t reg_val = readRegister(SW3538_REG_FORCE_OP2 + 1);
    reg_val &= ~(1 << adc_type);
    return writeRegister(SW3538_REG_FORCE_OP2 + 1, reg_val);
}

// 读取ADC数据 - 修复逻辑错误
uint16_t SW3538::readADCData(uint8_t channel) {
    if (!writeRegister(SW3538_REG_ADC_CONFIG, channel)) {
        return 0;
    }
    
    delay(5);  // ADC转换时间
    
    uint8_t low = readRegister(SW3538_REG_ADC_DATA_LOW);
    uint8_t high = readRegister(SW3538_REG_ADC_DATA_HIGH);
    
    if (channel == 11) {
        // 14位分辨率
        return ((uint16_t)(high & 0x7F) << 8) | low;
    } else {
        // 12位分辨率
        return ((uint16_t)(high & 0x0F) << 8) | low;
    }
}

// 读取所有数据 - 优化实现
bool SW3538::readAllData() {
    // 读取基础信息
    uint8_t version = readRegister(SW3538_REG_VERSION);
    data.chipVersion = version & 0x03;
    
    uint8_t power = readRegister(SW3538_REG_MAX_POWER);
    data.maxPowerW = power & 0x7F;
    
    // 检查通信状态
    if (version == 0xFF && power == 0xFF) {
        SW3538_LOG("I2C communication failed");
        return false;
    }
    
    // 读取快充状态
    uint8_t fc_reg = readRegister(SW3538_REG_FAST_CHARGE_IND);
    data.fastChargeStatus = ((fc_reg >> 7) & 0x01) || ((fc_reg >> 6) & 0x01);
    data.pdVersion = (fc_reg >> 4) & 0x03;
    data.fastChargeProtocol = (SW3538_FastChargeProtocol)(fc_reg & 0x0F);
    
    // 读取系统状态
    uint8_t status0 = readRegister(SW3538_REG_SYS_STATUS0);
    data.path1BuckStatus = (status0 >> 0) & 0x01;
    data.path2BuckStatus = (status0 >> 1) & 0x01;
    
    uint8_t status1 = readRegister(SW3538_REG_SYS_STATUS1);
    data.path1Online = (status1 >> 1) & 0x01;
    data.path2Online = (status1 >> 0) & 0x01;
    
    // 启用ADC通道
    enableADC(6);  // 输入电压
    enableADC(5);  // 输出电压
    enableADC(2);  // 通路2电流
    enableADC(1);  // 通路1电流
    enableADC(7);  // NTC
    
    // 读取ADC数据
    data.currentPath1mA = readADCData(1) * 2.5f;
    data.currentPath2mA = readADCData(2) * 2.5f;
    data.inputVoltagemV = readADCData(6) * 10.0f;
    data.outputVoltagemV = readADCData(11) * 1.0f;
    
    // 读取NTC温度
    uint16_t ntc_adc = readADCData(7);
    float ntc_voltage = ntc_adc * 1.2f;  // 1.2mV/bit
    
    uint8_t ntc_state = readRegister(SW3538_REG_NTC_CURRENT_STATE);
    float ntc_current = (ntc_state & 0x80) ? 40.0f : 20.0f;  // uA
    
    float ntc_resistance = ntc_voltage / ntc_current;  // kOhm
    
    // 温度计算 - 使用更精确的公式
    const float B = 3950.0f;
    const float T0 = 298.15f;
    const float R0 = 10.0f;
    
    float temp_k = 1.0f / (1.0f/T0 + (1.0f/B) * log(ntc_resistance/R0));
    data.ntcTemperatureC = temp_k - 273.15f;
    
    // 数据有效性检查
    if (data.ntcTemperatureC < 0 || data.ntcTemperatureC > 100) {
        data.ntcTemperatureC = -999;  // 无效值
    }
    
    // 禁用ADC
    disableADC(6);
    disableADC(5);
    disableADC(2);
    disableADC(1);
    disableADC(7);
    
    return true;
}

// 打印所有数据 - 使用固定格式
void SW3538::printAllData(Print& serial) {
    serial.println("--- SW3538 ---");
    serial.print("Version: "); serial.println(data.chipVersion);
    serial.print("MaxPower: "); serial.print(data.maxPowerW); serial.println("W");
    serial.print("FastCharger: "); serial.println(data.fastChargeStatus ? "ON" : "OFF");
    serial.print("Protocol: "); serial.println(getProtocolName(data.fastChargeProtocol));
    serial.print("PD_Version: "); serial.println(data.pdVersion == 1 ? "2.0" : (data.pdVersion == 2 ? "3.0" : "RSV"));
    serial.print("Path1 Link: "); serial.print(data.path1Online ? "ON" : "OFF");
    serial.print(" Path1 Buck:"); serial.println(data.path1BuckStatus ? "ON" : "OFF");
    serial.print("Path2 Link: "); serial.print(data.path2Online ? "ON" : "OFF");
    serial.print(" Path2 Buck:"); serial.println(data.path2BuckStatus ? "ON" : "OFF");
    serial.print("Path1 Current: "); serial.print(data.currentPath1mA); serial.println("mA");
    serial.print("Path2 Current: "); serial.print(data.currentPath2mA); serial.println("mA");
    serial.print("Input Voltage: "); serial.print(data.inputVoltagemV); serial.println("mV");
    serial.print("Output Voltage: "); serial.print(data.outputVoltagemV); serial.println("mV");
    serial.print("Temperature: "); 
    if (data.ntcTemperatureC == -999) {
        serial.println("N/A");
    } else {
        serial.print(data.ntcTemperatureC); serial.println("C");
    }
    serial.println("--------------");
}

// 设置函数 - 简化实现
bool SW3538::setNTC(uint8_t current_state) {
    if (current_state > 1) return false;
    
    if (!enableI2CWrite()) return false;
    
    uint8_t reg_val = readRegister(SW3538_REG_NTC_CURRENT_STATE);
    reg_val = (reg_val & 0x7F) | (current_state << 7);
    return writeRegister(SW3538_REG_NTC_CURRENT_STATE, reg_val);
}

bool SW3538::setMOSInternalResistance(uint8_t mos_setting) {
    if (mos_setting > 3) return false;
    
    if (!enableI2CWrite()) return false;
    
    uint8_t reg_val = readRegister(SW3538_REG_MOS_SETTING);
    reg_val = (reg_val & 0x3F) | (mos_setting << 6);
    return writeRegister(SW3538_REG_MOS_SETTING, reg_val);
}

bool SW3538::setNTCOverTempThreshold(uint8_t threshold_setting) {
    if (threshold_setting > 7) return false;
    
    if (!enableI2CWrite()) return false;
    
    uint8_t reg_val = readRegister(SW3538_REG_TEMP_SETTING);
    reg_val = (reg_val & 0xC7) | (threshold_setting << 3);
    return writeRegister(SW3538_REG_TEMP_SETTING, reg_val);
}
