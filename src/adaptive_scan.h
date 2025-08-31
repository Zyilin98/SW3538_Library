#ifndef ADAPTIVE_SCAN_H
#define ADAPTIVE_SCAN_H

#include <Arduino.h>

class AdaptiveScan {
public:
    void begin();              // 初始化
    bool tick();               // 放 loop 里，true=该扫了
    void notifyChange();       // 电流突变时调用
    void setEpsilon(float ma) { _eps = ma; }   // 电流变化阈值 mA
    void setBackoff(uint8_t k) { _backoff = k; }
    void setMaxInterval(uint32_t ms) { _maxInterval = ms; }  // 设置最大延时
    void updateCurrent(float i_ma); // 更新电流并自适应调整扫描周期
    void updateState(bool fastChargeStatus, bool path1Online, bool path2Online); // 多维状态检测
    
    // 调试和状态获取函数
    uint32_t getCurrentInterval() const { return _interval; }
    float getLastCurrent() const { return _lastI; }
    uint8_t getStableCount() const { return _stableCnt; }
    uint32_t getMaxInterval() const { return _maxInterval; }

private:
    uint32_t _interval;
    uint32_t _lastTick;
    float    _lastI = 0.0f;
    float    _eps   = 50.0f;   // 默认 50 mA 算变化
    uint8_t  _stableCnt = 0;
    uint8_t  _backoff = 2;
    uint32_t _maxInterval = 4000;  // 默认最大延时10秒
    
    // 状态跟踪变量
    bool _lastFastChargeStatus = false;
    bool _lastPath1Online = false;
    bool _lastPath2Online = false;
};

#endif
