#include "global_data.h"

/**
 * @brief SW3538数据全局实例
 * 
 * 此变量存储SW3538芯片的所有数据，包括电压、电流、功率等信息
 * 应通过getSW3538Data()函数访问，避免直接操作
 */
SW3538_Data_t sw3538Data;

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