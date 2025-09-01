#include "adaptive_scan.h"

/**
 * @brief 初始化自适应扫描器
 * 
 * 设置初始参数：
 * - 初始扫描间隔：200ms（上电后快速响应）
 * - 稳定计数器：0（刚开始计数）
 * - 上次电流值：0mA（初始状态）
 */
void AdaptiveScan::begin() {
    _interval  = 200;          // 上电先 200 ms，确保快速响应
    _lastTick  = millis();     // 记录初始化时间
    _stableCnt = 0;            // 稳定状态计数器清零
    _lastI = 0.0f;             // 初始电流设为0
}

/**
 * @brief 检查是否应该执行扫描
 * 
 * 基于时间间隔的非阻塞检查方法：
 * - 返回true：表示到达下次扫描时间
 * - 返回false：表示还未到时间，继续等待
 * 
 * @return true 应该执行扫描
 * @return false 继续等待
 */// 检查是否应该执行扫描
bool AdaptiveScan::tick() {
    if (millis() - _lastTick < _interval) return false;
    _lastTick = millis();  // 更新时间戳
    
    // 打印当前刷新间隔（调试信息）
    Serial.print("[AdaptiveScan] Current interval: ");
    Serial.print(_interval);
    Serial.println("ms");
    return true;
}

/**
 * @brief 立即切换到高速扫描模式
 * 
 * 当检测到重要变化时调用：
 * - 电流突变（充电开始/结束）
 * - 快充协议建立
 * - 设备插入/拔出
 * 
 * 将扫描间隔重置为200ms，确保快速响应
 */
void AdaptiveScan::notifyChange() {
    _interval  = 200;          // 立即回到高速扫描（200ms间隔）
    _stableCnt = 0;            // 重置稳定计数器，重新开始计数
}

/**
 * @brief 基于电流变化调整扫描频率的核心算法
 * 
 * 实现原理：
 * 1. 变化检测：比较当前电流与上次电流的差值
 * 2. 自适应调整：根据变化幅度动态调整扫描频率
 * 3. 指数退避：稳定状态下逐步降低扫描频率
 * 
 * @param i_ma 当前总电流值（mA）
 */
void AdaptiveScan::updateCurrent(float i_ma) {
    // 数据预处理：确保电流值为非负数
    if (i_ma < 0.0f) i_ma = 0.0f;  // 防止负值，保护算法
    
    // 阶段1：检测电流突变
    if (fabs(i_ma - _lastI) > _eps) {
        notifyChange();  // 电流变化超过阈值，立即恢复高速扫描
    } 
    // 阶段2：稳定状态检测
    else if (++_stableCnt >= 5) {  // 连续5次稳定后考虑减速
        // 指数退避算法：每次乘以退避系数（默认2倍）
        uint32_t newInterval = (uint32_t)(_interval * _backoff);
        
        // 边界保护：限制扫描间隔范围
        if (newInterval < 200) newInterval = 200;    // 最小200ms（快速响应）
        if (newInterval > 5000) newInterval = 5000;    // 最大5秒（节能模式）
        
        _interval = newInterval;  // 应用新的扫描间隔
        _stableCnt = 0;          // 重置计数器，重新开始检测
    }
    
    _lastI = i_ma;  // 保存当前电流值作为下次比较基准
}

/**
 * @brief 多维状态变化检测
 * 
 * 监控除电流外的其他关键状态：
 * - 快充协议状态（PD/QC等协议建立）
 * - 通路连接状态（设备插入/拔出）
 * - 充电状态变化（开始/停止充电）
 * 
 * 任何状态变化都会触发高速扫描，确保及时响应
 * 
 * @param fastChargeStatus 快充状态（true=快充协议已建立）
 * @param path1Online 第一通路连接状态（true=设备已连接）
 * @param path2Online 第二通路连接状态（true=设备已连接）
 */
void AdaptiveScan::updateState(bool fastChargeStatus, bool path1Online, bool path2Online) {
    // 初始化状态变化标志
    bool stateChanged = false;
    
    // 检测1：快充协议状态变化
    // 当设备开始快充或退出快充时，需要快速响应
    if (fastChargeStatus != _lastFastChargeStatus) {
        stateChanged = true;
    }
    
    // 检测2：设备连接状态变化
    // 当设备插入或拔出时，需要立即调整扫描频率
    if (path1Online != _lastPath1Online || path2Online != _lastPath2Online) {
        stateChanged = true;
    }
    
    // 触发高速扫描
    if (stateChanged) {
        notifyChange();  // 立即切换到高速扫描模式
    }
    
    // 更新状态缓存，为下次检测做准备
    _lastFastChargeStatus = fastChargeStatus;
    _lastPath1Online = path1Online;
    _lastPath2Online = path2Online;
}