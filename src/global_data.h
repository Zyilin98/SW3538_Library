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
 * @brief 存储SW3538芯片的全局数据
 * 
 * 此变量用于存储从SW3538芯片读取的数据，供系统各模块使用
 * 建议通过getSW3538Data()函数访问，而不是直接访问此变量
 */
extern SW3538_Data_t sw3538Data;

/**
 * @brief 获取SW3538数据的接口函数
 * 
 * @return const SW3538_Data_t& 返回SW3538数据的常量引用
 * @note 此函数提供了对全局数据的只读访问，确保数据不被意外修改
 */
const SW3538_Data_t& getSW3538Data();

/**
 * @brief 检查SW3538数据是否有效
 * 
 * @return bool 如果数据有效返回true，否则返回false
 * @note 数据有效性检查基于芯片版本和最大功率参数
 */
bool isSW3538DataValid();

#endif