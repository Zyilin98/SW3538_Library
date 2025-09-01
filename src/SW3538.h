/*
 * SW3538.h - 精简版SW3538驱动头文件
 * 
 * 改写说明：
 * 1. 移除所有String类依赖
 * 2. 添加协议名称查找表（无String）
 * 3. 优化枚举定义
 * 4. 保持接口兼容性
 */

#ifndef SW3538_H
#define SW3538_H

#include <Arduino.h>
#include <Wire.h>

// SW3538寄存器定义
#define SW3538_DEFAULT_ADDRESS     0x3C
#define SW3538_REG_VERSION          0x00
#define SW3538_REG_MAX_POWER        0x02
#define SW3538_REG_FAST_CHARGE_IND  0x09
#define SW3538_REG_SYS_STATUS0      0x0A
#define SW3538_REG_SYS_STATUS1      0x0D
#define SW3538_REG_I2C_ENABLE       0x10
#define SW3538_REG_FORCE_OP_ENABLE  0x15
#define SW3538_REG_FORCE_OP2        0x18
#define SW3538_REG_ADC_CONFIG       0x40
#define SW3538_REG_ADC_DATA_LOW     0x41
#define SW3538_REG_ADC_DATA_HIGH    0x42
#define SW3538_REG_NTC_CURRENT_STATE 0x44
#define SW3538_REG_MOS_SETTING      0x107
#define SW3538_REG_TEMP_SETTING     0x10D

// 调试开关 - 设置为0可完全关闭调试信息
#define SW3538_DEBUG 1

#if SW3538_DEBUG
    #define SW3538_LOG(msg) Serial.println(msg)
    #define SW3538_LOG_VAL(msg, val) do { Serial.print(msg); Serial.println(val); } while(0)
#else
    #define SW3538_LOG(msg)
    #define SW3538_LOG_VAL(msg, val)
#endif

// 快充协议枚举
enum SW3538_FastChargeProtocol {
    SW3538_FC_NONE = 0,
    SW3538_FC_QC2_0 = 1,
    SW3538_FC_QC3_0 = 2,
    SW3538_FC_QC3_PLUS = 3,
    SW3538_FC_FCP = 4,
    SW3538_FC_SCP = 5,
    SW3538_FC_PD_FIX = 6,
    SW3538_FC_PD_PPS = 7,
    SW3538_FC_PE1_1 = 8,
    SW3538_FC_PE2_0 = 9,
    SW3538_FC_VOOC1_0 = 10,
    SW3538_FC_VOOC4_0 = 11,
    SW3538_FC_SFCP = 13,
    SW3538_FC_AFC = 14,
    SW3538_FC_TFCP = 15
};

// 数据结构 - 优化字段顺序以减少内存对齐开销
typedef struct {
    uint16_t inputVoltagemV;
    uint16_t outputVoltagemV;
    int16_t currentPath1mA;
    int16_t currentPath2mA;
    int16_t ntcTemperatureC;
    uint16_t maxPowerW;
    uint8_t chipVersion;
    uint8_t pdVersion;
    SW3538_FastChargeProtocol fastChargeProtocol;
    bool fastChargeStatus;
    bool path1Online;
    bool path2Online;
    bool path1BuckStatus;
    bool path2BuckStatus;
} SW3538_Data_t;

// 协议名称查找表 - 无String实现
class SW3538 {
public:
    // 构造函数
    SW3538(uint8_t address = SW3538_DEFAULT_ADDRESS);
    SW3538(uint8_t address, int sdaPin, int sclPin);
    
    // 基本功能
    void begin();
    bool testI2CAddress(uint8_t address);
    void scanI2CAddresses();
    bool readAllData();
    void printAllData(Print& serial);
    
    // 设置功能
    bool setNTC(uint8_t current_state); // 0:20uA, 1:40uA
    bool setMOSInternalResistance(uint8_t mos_setting); // 0-3
    bool setNTCOverTempThreshold(uint8_t threshold_setting); // 0-7
    
    // 静态方法 - 获取协议名称（无String）
    static const char* getProtocolName(SW3538_FastChargeProtocol protocol) {
        static const char* names[] = {
            "NONE", "QC2.0", "QC3.0", "QC3+", "FCP", "SCP", 
            "PD-FIX", "PD-PPS", "PE1.1", "PE2.0", "VOOC1", 
            "VOOC4", "RSV", "SFCP", "AFC", "TFCP"
        };
        if (protocol <= 15) {
            return names[protocol];
        }
        return "UNKNOWN";
    }
    
    // 公共数据访问
    SW3538_Data_t data;

private:
    uint8_t _address;
    int _sdaPin;
    int _sclPin;
    bool _useCustomPins;
    
    // 私有方法
    uint8_t readRegister(uint16_t reg);
    bool writeRegister(uint16_t reg, uint8_t value);
    bool enableI2CWrite();
    bool enableForceOperationWrite();
    bool enableADC(uint8_t adc_type);
    bool disableADC(uint8_t adc_type);
    uint16_t readADCData(uint8_t channel);
};

#endif // SW3538_H
