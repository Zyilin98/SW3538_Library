#ifndef ADAPTIVE_SCAN_H
#define ADAPTIVE_SCAN_H

#include <Arduino.h>

/**
 * @class AdaptiveScan
 * @brief 自适应扫描频率控制器
 * 
 * 实现原理：
 * 1. 变化检测：监控电流、快充状态、设备连接等多维度变化
 * 2. 频率调整：根据变化幅度动态调整扫描频率（200ms-5s）
 * 3. 指数退避：稳定状态下逐步降低频率，变化时立即提速
 * 
 * 优势：
 * - 变化时快速响应（200ms）
 * - 稳定时节能降频（最长5s）
 * - 非阻塞设计，不影响主循环
 */
class AdaptiveScan {
public:
    /**
     * @brief 初始化自适应扫描器
     * 
     * 设置初始扫描间隔为200ms，确保上电后能快速响应
     */
    void begin();
    
    /**
     * @brief 检查是否应该执行扫描
     * 
     * 非阻塞检查方法，应在主循环中持续调用
     * 
     * @return true 到达下次扫描时间，应执行扫描
     * @return false 还未到时间，继续等待
     */
    bool tick();
    
    /**
     * @brief 强制切换到高速扫描模式
     * 
     * 当检测到重要变化时调用，立即将扫描间隔重置为200ms
     */
    void notifyChange();
    
    /**
     * @brief 设置电流变化检测阈值
     * 
     * @param ma 电流变化阈值，单位mA，默认50mA
     */
    void setEpsilon(float ma) { _eps = ma; }
    
    /**
     * @brief 设置退避系数
     * 
     * @param k 退避系数，默认2（每次稳定后间隔×2）
     */
    void setBackoff(uint8_t k) { _backoff = k; }
    
    /**
     * @brief 设置最大扫描间隔
     * 
     * @param ms 最大间隔时间，单位ms，默认5000ms（5秒）
     */
    void setMaxInterval(uint32_t ms) { _maxInterval = ms; }
    
    /**
     * @brief 基于电流变化调整扫描频率
     * 
     * 核心算法：
     * - 电流变化超过阈值 → 立即提速到200ms
     * - 连续5次稳定 → 逐步增加间隔（×退避系数）
     * - 限制范围：200ms-5000ms
     * 
     * @param i_ma 当前总电流值，单位mA
     */
    void updateCurrent(float i_ma);
    
    /**
     * @brief 多维状态变化检测
     * 
     * 监控快充协议和设备连接状态变化
     * 
     * @param fastChargeStatus 快充协议状态（true=已建立快充）
     * @param path1Online 第一通路连接状态（true=设备已连接）
     * @param path2Online 第二通路连接状态（true=设备已连接）
     */
    void updateState(bool fastChargeStatus, bool path1Online, bool path2Online);
    
    // ===== 调试和状态获取函数 =====
    
    /**
     * @brief 获取当前扫描间隔
     * @return 当前扫描间隔，单位ms
     */
    uint32_t getCurrentInterval() const { return _interval; }
    
    /**
     * @brief 获取上次记录的电流值
     * @return 上次电流值，单位mA
     */
    float getLastCurrent() const { return _lastI; }
    
    /**
     * @brief 获取当前稳定计数
     * @return 连续稳定次数
     */
    uint8_t getStableCount() const { return _stableCnt; }
    
    /**
     * @brief 获取最大扫描间隔设置
     * @return 最大扫描间隔，单位ms
     */
    uint32_t getMaxInterval() const { return _maxInterval; }

private:
    // ===== 核心控制参数 =====
    uint32_t _interval;      // 当前扫描间隔，动态调整
    uint32_t _lastTick;      // 上次扫描时间戳
    float    _lastI = 0.0f;  // 上次记录的电流值（mA）
    
    // ===== 算法参数 =====
    float    _eps   = 50.0f;   // 电流变化阈值，默认50mA
    uint8_t  _stableCnt = 0;   // 连续稳定计数器
    uint8_t  _backoff = 2;     // 退避系数，稳定后×2
    uint32_t _maxInterval = 5000;  // 最大扫描间隔，默认5000ms（5秒）
    
    // ===== 状态跟踪变量 =====
    bool _lastFastChargeStatus = false;  // 上次快充状态
    bool _lastPath1Online = false;     // 上次第一通路状态
    bool _lastPath2Online = false;     // 上次第二通路状态
};

#endif
