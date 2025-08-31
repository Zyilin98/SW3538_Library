#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "SW3538.h"

/**
 * @brief 全局数据管理模块
 * 
 * 该模块负责管理系统中的全局数据，提供统一的访问接口
 * 避免直接访问全局变量，提高代码的可维护性和可测试性
 */

/**
 * @brief 计算后的显示数据
 * 
 * 存储从SW3538原始数据计算得到的显示用数据
 * 避免重复计算，提高显示效率
 */
struct DisplayData {
    float inputVoltage;    // 输入电压 (V)
    float outputVoltage;   // 输出电压 (V)
    float current1;        // 通路1电流 (A)
    float current2;        // 通路2电流 (A)
    float totalCurrent;    // 总电流 (A)
    float power;           // 总功率 (W)
};

/**
 * @brief 存储SW3538芯片的全局数据
 * 
 * 此变量用于存储从SW3538芯片读取的数据，供系统各模块使用
 * 建议通过getSW3538Data()函数访问，而不是直接访问此变量
 */
extern SW3538_Data_t sw3538Data;

/**
 * @brief 存储计算后的显示数据
 * 
 * 此变量存储从SW3538数据计算得到的显示用数据
 * 避免每次显示时重复计算
 */
extern DisplayData displayData;

/**
 * @brief 获取SW3538数据的接口函数
 * 
 * @return const SW3538_Data_t& 返回SW3538数据的常量引用
 * @note 此函数提供了对全局数据的只读访问，确保数据不被意外修改
 */
const SW3538_Data_t& getSW3538Data();

/**
 * @brief 获取显示数据的接口函数
 * 
 * @return const DisplayData& 返回显示数据的常量引用
 * @note 此函数提供了对计算后数据的只读访问
 */
const DisplayData& getDisplayData();

/**
 * @brief 检查SW3538数据是否有效
 * 
 * @return bool 如果数据有效返回true，否则返回false
 * @note 数据有效性检查基于芯片版本和最大功率参数
 */
bool isSW3538DataValid();

/**
 * @brief 更新显示数据
 * 
 * 从SW3538原始数据计算显示所需的各种参数
 * @param data SW3538原始数据
 * @note 此函数会自动更新全局displayData变量
 */
void updateDisplayData(const SW3538_Data_t& data);
/**
 * @brief 打印显示数据到串口（调试用途）
 * 
 * 用于调试和验证显示数据的正确性
 */
void printDisplayData();

#endif