#include "adaptive_scan.h"

void AdaptiveScan::begin() {
    _interval  = 200;          // 上电先 200 ms
    _lastTick  = millis();
    _stableCnt = 0;
    _lastI = 0.0f;
}

bool AdaptiveScan::tick() {
    if (millis() - _lastTick < _interval) return false;
    _lastTick = millis();
    return true;
}

void AdaptiveScan::notifyChange() {
    _interval  = 200;          // 立即回到高速
    _stableCnt = 0;
}

// 在每次读完电流后调用
void AdaptiveScan::updateCurrent(float i_ma) {
    // 检查是否为有效电流值
    if (i_ma < 0.0f) i_ma = 0.0f;  // 防止负值
    
    if (fabs(i_ma - _lastI) > _eps) {
        notifyChange();  // 电流突变，立即恢复高速扫描
    } else if (++_stableCnt >= 5) {  // 恢复为5次稳定后减速
        // 限制最大间隔不超过5秒，最小间隔不小于100ms
        uint32_t newInterval = (uint32_t)(_interval * _backoff);
        if (newInterval < 200) newInterval = 200;
        if (newInterval > 5000) newInterval = 5000;
        _interval = newInterval;
        _stableCnt = 0;
    }
    _lastI = i_ma;
}

// 多维状态突变检测
void AdaptiveScan::updateState(bool fastChargeStatus, bool path1Online, bool path2Online) {
    // 检测关键状态变化
    bool stateChanged = false;
    
    // 快充状态变化（协议建立）
    if (fastChargeStatus != _lastFastChargeStatus) {
        stateChanged = true;
    }
    
    // 通路状态变化（设备插入/拔出）
    if (path1Online != _lastPath1Online || path2Online != _lastPath2Online) {
        stateChanged = true;
    }
    
    if (stateChanged) {
        notifyChange();  // 立即触发高速扫描
    }
    
    // 更新状态缓存
    _lastFastChargeStatus = fastChargeStatus;
    _lastPath1Online = path1Online;
    _lastPath2Online = path2Online;
}