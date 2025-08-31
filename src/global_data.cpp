#include "global_data.h"
#include <Arduino.h>  // 用于max函数和Serial

/**
 * @brief SW3538数据全局实例
 * 
 * 此变量存储SW3538芯片的所有数据，包括电压、电流、功率等信息
 * 应通过getSW3538Data()函数访问，避免直接操作
 */
SW3538_Data_t sw3538Data;

/**
 * @brief 显示数据全局实例
 * 
 * 此变量存储计算后的显示数据，避免重复计算
 */
DisplayData displayData = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

/**
 * @brief 获取SW3538数据的实现
 * 
 * @return const SW3538_Data_t& 返回SW3538数据的常量引用
 * @note 此函数提供了对全局数据的只读访问，确保数据一致性
 */
const SW3538_Data_t& getSW3538Data() {
    return sw3538Data;
}

/**
 * @brief 获取显示数据的实现
 * 
 * @return const DisplayData& 返回显示数据的常量引用
 * @note 此函数提供了对计算后数据的只读访问
 */
const DisplayData& getDisplayData() {
    return displayData;
}

/**
 * @brief 检查SW3538数据有效性的实现
 * 
 * @return bool 如果数据有效返回true，否则返回false
 * @details 有效性检查规则：
 *          1. 芯片版本必须小于等于3
 *          2. 最大功率必须小于等于65W
 * @note 这些规则基于SW3538芯片的规格参数制定
 */
bool isSW3538DataValid() {
    return (sw3538Data.chipVersion <= 3) && (sw3538Data.maxPowerW <= 65);
}

/**
 * @brief 更新显示数据
 * 
 * 从SW3538原始数据计算显示所需的各种参数
 * @param data SW3538原始数据
 * @note 此函数会自动更新全局displayData变量
 */
void updateDisplayData(const SW3538_Data_t& data) {
    // 计算电压（单位：V）
    displayData.inputVoltage = data.inputVoltagemV / 1000.0f;
    displayData.outputVoltage = data.outputVoltagemV / 1000.0f;
    
    // 计算电流（单位：A）
    displayData.current1 = data.currentPath1mA / 1000.0f;
    displayData.current2 = data.currentPath2mA / 1000.0f;
    displayData.totalCurrent = displayData.current1 + displayData.current2;
    
    // 计算功率（单位：W），防止负值或异常值
    displayData.power = max(0.0f, displayData.outputVoltage * displayData.totalCurrent);
    
}

/**
 * @brief 打印显示数据到串口（调试用途）
 * 
 * 用于调试和验证显示数据的正确性
 */
void printDisplayData() {
    Serial.println("=== 显示数据 ===");
    Serial.print("输入电压: ");
    Serial.print(displayData.inputVoltage);
    Serial.println(" V");
    
    Serial.print("输出电压: ");
    Serial.print(displayData.outputVoltage);
    Serial.println(" V");
    
    Serial.print("通路1电流: ");
    Serial.print(displayData.current1);
    Serial.println(" A");
    
    Serial.print("通路2电流: ");
    Serial.print(displayData.current2);
    Serial.println(" A");
    
    Serial.print("总电流: ");
    Serial.print(displayData.totalCurrent);
    Serial.println(" A");
    
    Serial.print("总功率: ");
    Serial.print(displayData.power);
    Serial.println(" W");

}