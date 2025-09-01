#include <Arduino.h>
#include <Wire.h>
#include "SW3538.h"
#include "global_data.h"
#include "display.h"
#include "adaptive_scan.h"

// SW3538实例
SW3538 sw3538;
AdaptiveScan aScan;

// 函数声明
void displaySerialData();
void displaySystemInfo();
unsigned long getNonBlockingDelay(unsigned long lastTime, unsigned long interval);

void setup() {
    Serial.begin(115200);
    
    // 初始化OLED
    initOled();
    // 初始化防烧屏功能
    updateLastAccessTime(); // 设置初始访问时间
    
    // 系统信息
    Serial.println("系统信息:");
    Serial.print("MCU: ESP32-C3");
    Serial.print(" 时钟频率: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println("MHz");
    Serial.print("可用内存: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println();
    
    // 初始化SW3538
    Serial.println("初始化SW3538...");
    sw3538.begin();
    
    // 测试通信
    Serial.println("测试SW3538通信...");
    if (sw3538.testI2CAddress(0x3C)) {
        Serial.println("SW3538通信正常");
        
        // 读取初始数据
        if (sw3538.readAllData()) {
            Serial.println("初始数据读取成功");
            // 初始化显示数据
            sw3538Data = sw3538.data;
            updateDisplayData(sw3538Data);
        } else {
            Serial.println("初始数据读取失败");
        }
    }
    /**
     * @brief 初始化自适应扫描系统
     * 
     * 配置参数说明：
     * - begin()：设置初始扫描间隔200ms，确保上电快速响应
     * - setEpsilon(50)：设置电流变化阈值为50mA
     *   - 当电流变化超过50mA时，立即切换到高速模式
     *   - 适用于手机充电等场景，既能检测充电开始，又避免微小波动干扰
     * 
     * 算法优势：
     * - 变化时快速响应（200ms），及时显示充电状态
     * - 稳定时降频节能（最长5s），减少CPU占用和功耗
     * - 自动适应不同充电场景（涓流、快充、满电）
     */
    aScan.begin();
    aScan.setEpsilon(50);
}

void loop() {
    // 检查按钮状态
    checkButtonState();
    checkOledTimeout();
    /**
     * @brief 自适应数据读取和显示主循环
     * 
     * 工作流程：
     * 1. 检查扫描时机：aScan.tick()根据自适应算法决定是否扫描
     * 2. 读取设备数据：当tick()返回true时，读取SW3538寄存器数据
     * 3. 更新自适应算法：
     *    - updateCurrent()：基于总电流变化调整扫描频率
     *    - updateState()：基于快充和设备连接状态调整扫描频率
     * 4. 更新显示：将新数据显示到OLED和串口
     * 
     * 自适应行为示例：
     * - 手机插入充电：电流从0→500mA，立即提速到200ms
     * - 稳定充电中：逐步降频到1-5秒，节能运行
     * - 快充协议建立：状态变化触发，立即提速观察
     * - 设备拔出：电流突变，快速响应显示0mA
     */
    
    // 步骤1：检查是否应该执行扫描（自适应频率控制）
    if (aScan.tick()){
        
        // 步骤2：读取SW3538完整数据
        if (sw3538.readAllData()) {
            // 调试输出：通过串口显示所有寄存器数据
            sw3538.printAllData(Serial);
            
            // 步骤3：计算总电流（两路之和）
            float total_ma = sw3538.data.currentPath1mA +
                             sw3538.data.currentPath2mA;
            
            // 步骤4：更新自适应算法
            // 4.1 基于电流变化调整扫描频率
            aScan.updateCurrent(total_ma);
            
            // 4.2 基于多维状态变化调整扫描频率
            aScan.updateState(sw3538.data.fastChargeStatus, 
                             sw3538.data.path1Online, 
                             sw3538.data.path2Online);
            
            // 步骤5：更新全局数据结构
            sw3538Data = sw3538.data;  // 供其他模块使用
            
            // 步骤6：计算并存储显示数据（电压、电流、功率等）
            updateDisplayData(sw3538Data);
            
            // 步骤7：刷新OLED显示
            displaySw3538Data();
            
        } else {
            // 错误处理：数据读取失败
            Serial.println("[ERROR] 数据读取失败");
        }
    }
}
