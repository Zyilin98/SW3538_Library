#include <Arduino.h>
#include "SW3538.h"
#include <Wire.h>

/*
 * SW3538 驱动日志系统说明
 * 
 * 日志级别 (数字越小，级别越低，输出越少):
 * - LOG_LEVEL_INFO (1): 基本信息日志，用于记录系统运行关键状态
 *   例如：初始化过程、通信状态、数据读取结果等
 * - LOG_LEVEL_WARNING (2): 警告日志，用于记录可能需要注意的情况
 *   例如：读取到异常值、通信超时等不影响系统运行但需要关注的问题
 * - LOG_LEVEL_DEBUG (3): 调试日志，用于开发和调试过程
 *   例如：寄存器读写值、详细的状态变化、函数调用过程等
 * - LOG_LEVEL_ERROR (4): 错误日志，用于记录系统错误
 *   例如：通信失败、初始化失败等影响系统正常运行的问题
 * 
 * 使用方法:
 * 1. 修改 CURRENT_LOG_LEVEL 宏定义来设置当前日志级别
 *    - 开发调试阶段: 建议设置为 LOG_LEVEL_DEBUG
 *    - 生产运行阶段: 建议设置为 LOG_LEVEL_INFO 或 LOG_LEVEL_ERROR
 * 2. 使用对应的日志宏输出不同级别的日志:
 *    - LOG_INFO(message): 输出信息日志，包含[INFO]标识和函数名
 *    - LOG_WARNING(message): 输出警告日志，包含[WARNING]标识和函数名
 *    - LOG_DEBUG(message): 输出调试日志，包含[DEBUG]标识和函数名
 *    - LOG_ERROR(message): 输出错误日志，包含[ERROR]标识和函数名
 * 3. 日志会自动包含函数名和日志级别标识，方便定位问题
 * 
 * 示例:
 * LOG_INFO("初始化成功");  // 输出信息日志
 * LOG_WARNING("电压超出正常范围");  // 输出警告日志
 * LOG_DEBUG("读取寄存器值: 0x" + String(value, HEX));  // 输出调试日志
 * LOG_ERROR("通信失败");  // 输出错误日志
 */
// 定义调试日志级别 - 生产环境建议设置为 LOG_LEVEL_INFO 或 LOG_LEVEL_ERROR
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_DEBUG   3
#define LOG_LEVEL_ERROR   4
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG

// 调试日志宏
#define LOG_INFO(message)  if(CURRENT_LOG_LEVEL >= LOG_LEVEL_INFO)  Serial.println("[INFO] " + String(__func__) + ": " + message)
#define LOG_WARNING(message) if(CURRENT_LOG_LEVEL >= LOG_LEVEL_WARNING) Serial.println("[WARNING] " + String(__func__) + ": " + message)
#define LOG_DEBUG(message) if(CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) Serial.println("[DEBUG] " + String(__func__) + ": " + message)
#define LOG_ERROR(message) if(CURRENT_LOG_LEVEL >= LOG_LEVEL_ERROR) Serial.println("[ERROR] " + String(__func__) + ": " + message)

// Constructor for hardware I2C with default address
SW3538::SW3538(uint8_t address) : _address(address) {
    LOG_INFO("SW3538 initialized with I2C address: 0x" + String(_address, HEX));
}

// Test specific I2C address
bool SW3538::testI2CAddress(uint8_t address) {
  Serial.print("Testing address 0x");
  if (address < 0x10) Serial.print("0");
  Serial.print(address, HEX);
  Serial.print("... ");
  
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();
  bool success = (error == 0);
  
  if (success) {
    Serial.println("✓ RESPONDING");
    
    // Try to read a register to verify it's actually SW3538
    uint8_t testReg = readRegister(SW3538_REG_VERSION);
    if (testReg != 0xFF && testReg != 0x00) {
      Serial.println("  ✓ SW3538 detected (register read successful)");
      return true;
    } else {
      Serial.println("  ⚠ Device responding but data invalid (0x" + String(testReg, HEX) + ")");
      return false;
    }
  } else {
    Serial.print("✗ ");
    switch(error) {
      case 1: Serial.println("BUS ERROR"); break;
      case 2: Serial.println("NACK ADDR"); break;
      case 3: Serial.println("NACK DATA"); break;
      case 4: Serial.println("OTHER ERROR"); break;
      default: Serial.println("UNKNOWN"); break;
    }
    return false;
  }
}

// Scan all possible I2C addresses
void SW3538::scanI2CAddresses() {
  LOG_INFO("Scanning I2C addresses...");
  byte error, address;
  int nDevices = 0;

  Serial.println("I2C Address Scan Results:");
  Serial.println("Address  Status     Description");
  Serial.println("--------  --------   -----------");

  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("0x");
      if (address < 0x10) Serial.print("0");
      Serial.print(address, HEX);
      Serial.print("    ✓ FOUND    ");
      
      if (address == 0x3C) Serial.println("SW3538 (A0=GND)");
      else if (address == 0x3D) Serial.println("SW3538 (A0=VCC)");
      else Serial.println("Unknown device");
      
      nDevices++;
    } else {
      Serial.print("0x");
      if (address < 0x10) Serial.print("0");
      Serial.print(address, HEX);
      Serial.print("    ✗ ");
      
      switch(error) {
        case 1: Serial.println("BUS ERROR  Data too long"); break;
        case 2: Serial.println("NACK ADDR  Address not acknowledged"); break;
        case 3: Serial.println("NACK DATA  Data not acknowledged"); break;
        case 4: Serial.println("OTHER      Other error"); break;
        default: Serial.println("UNKNOWN    Unknown error"); break;
      }
    }
  }
  
  Serial.println("--------");
  if (nDevices == 0) {
    LOG_ERROR("No I2C devices found - Check wiring and power!");
  } else {
    LOG_INFO("Found " + String(nDevices) + " I2C device(s)");
  }
}

void SW3538::begin() {
    LOG_INFO("Initializing SW3538...");
    
    // Initialize I2C
    Wire.begin();
    Wire.setClock(100000); // 100kHz I2C speed
    
    // Perform basic communication test
    LOG_INFO("Performing basic communication test...");
    uint8_t version = readRegister(SW3538_REG_VERSION);
    if (version == 0xFF || version == 0x00) {
        LOG_ERROR("Communication test failed - check I2C connections and pull-up resistors");
    } else {
        LOG_INFO("Communication test passed - chip version: " + String(version));
    }
    
    LOG_INFO("SW3538 initialized");
}

// Private function to read a single byte from a register
uint8_t SW3538::readRegister(uint16_t reg) {
    LOG_DEBUG("Reading register 0x" + String(reg, HEX) + " from address 0x" + String(_address, HEX));

    // SW3538 uses 8-bit register addresses, not 16-bit
    uint8_t reg_addr = (uint8_t)(reg & 0xFF);
    
    // Try up to 5 times with exponential backoff
    for (int retry = 0; retry < 5; retry++) {
        // Clear any pending I2C state
        Wire.endTransmission(true);
        
        Wire.beginTransmission(_address);
        Wire.write(reg_addr); // Single byte register address
        
        int error = Wire.endTransmission(false); // Send address without stop condition
        if (error != 0) {
            LOG_ERROR("Attempt " + String(retry+1) + ": Failed to send register address (error: " + String(error) + ")");
            delay(10 * (1 << retry)); // Exponential backoff delay (10ms, 20ms, 40ms, 80ms, 160ms)
            continue;
        }

        uint8_t bytesReceived = Wire.requestFrom(_address, (uint8_t)1);
        if (bytesReceived == 1 && Wire.available()) {
            uint8_t value = Wire.read();
            
            // Validate the data - 0xFF might indicate communication issue
            if (value == 0xFF) {
                LOG_WARNING("Register 0x" + String(reg, HEX) + " returned 0xFF - possible communication issue");
            }
            
            LOG_DEBUG("Success on attempt " + String(retry+1) + ": Read value 0x" + String(value, HEX) + " from register 0x" + String(reg, HEX));
            return value;
        } else {
            LOG_ERROR("Attempt " + String(retry+1) + ": No data available (received: " + String(bytesReceived) + ")");
            delay(10 * (1 << retry)); // Exponential backoff delay
        }
    }

    LOG_ERROR("All attempts failed to read register 0x" + String(reg, HEX));
    return 0xFF; // Return 0xFF to indicate communication failure
}

// Private function to write a single byte to a register
bool SW3538::writeRegister(uint16_t reg, uint8_t value) {
    LOG_DEBUG("Writing value 0x" + String(value, HEX) + " to register 0x" + String(reg, HEX) + " at address 0x" + String(_address, HEX));

    // SW3538 uses 8-bit register addresses, not 16-bit
    uint8_t reg_addr = (uint8_t)(reg & 0xFF);
    
    // Try up to 3 times
    for (int retry = 0; retry < 3; retry++) {
        Wire.beginTransmission(_address);
        Wire.write(reg_addr); // Single byte register address
        Wire.write(value);

        int error = Wire.endTransmission();
        if (error == 0) {
            LOG_DEBUG("Success on attempt " + String(retry+1) + ": Wrote to register 0x" + String(reg, HEX));
            return true;
        } else {
            LOG_ERROR("Attempt " + String(retry+1) + ": Failed to write (error: " + String(error) + ")");
            delay(5); // Small delay before retry
        }
    }

    LOG_ERROR("All 3 attempts failed to write to register 0x" + String(reg, HEX));
    return false;
}

// Sequence to enable I2C write operations (Reg0x10)
bool SW3538::enableI2CWrite() {
    LOG_INFO("Enabling I2C write operations");
    if (!writeRegister(SW3538_REG_I2C_ENABLE, 0x20)) {
        LOG_ERROR("Failed to write 0x20 to SW3538_REG_I2C_ENABLE");
        return false;
    }
    if (!writeRegister(SW3538_REG_I2C_ENABLE, 0x40)) {
        LOG_ERROR("Failed to write 0x40 to SW3538_REG_I2C_ENABLE");
        return false;
    }
    if (!writeRegister(SW3538_REG_I2C_ENABLE, 0x80)) {
        LOG_ERROR("Failed to write 0x80 to SW3538_REG_I2C_ENABLE");
        return false;
    }
    LOG_INFO("I2C write operations enabled successfully");
    return true;
}

// Sequence to enable force operation write (Reg0x15)
bool SW3538::enableForceOperationWrite() {
    LOG_INFO("Enabling force operation write");
    if (!writeRegister(SW3538_REG_FORCE_OP_ENABLE, 0x20)) {
        LOG_ERROR("Failed to write 0x20 to SW3538_REG_FORCE_OP_ENABLE");
        return false;
    }
    if (!writeRegister(SW3538_REG_FORCE_OP_ENABLE, 0x40)) {
        LOG_ERROR("Failed to write 0x40 to SW3538_REG_FORCE_OP_ENABLE");
        return false;
    }
    if (!writeRegister(SW3538_REG_FORCE_OP_ENABLE, 0x80)) {
        LOG_ERROR("Failed to write 0x80 to SW3538_REG_FORCE_OP_ENABLE");
        return false;
    }
    LOG_INFO("Force operation write enabled successfully");
    return true;
}

// Enable specific ADC channel for sampling (Reg0x18)
bool SW3538::enableADC(uint8_t adc_type) {
    LOG_DEBUG("Enabling ADC type: " + String(adc_type));

    // First, enable force operation write
    if (!enableForceOperationWrite()) {
        LOG_ERROR("Failed to enable force operation write");
        return false;
    }

    // Read current Reg0x18 value, set the bit, then write back
    uint8_t reg_val = readRegister(SW3538_REG_FORCE_OP2);
    LOG_DEBUG("Current Reg0x18 value: 0x" + String(reg_val, HEX));

    uint8_t new_reg_val = reg_val | (1 << adc_type);
    LOG_DEBUG("New Reg0x18 value: 0x" + String(new_reg_val, HEX));

    if (!writeRegister(SW3538_REG_FORCE_OP2, new_reg_val)) {
        LOG_ERROR("Failed to write to Reg0x18");
        return false;
    }

    LOG_INFO("Successfully enabled ADC type: " + String(adc_type));
    return true;
}

// Disable specific ADC channel for sampling (Reg0x19)
bool SW3538::disableADC(uint8_t adc_type) {
    LOG_DEBUG("Disabling ADC type: " + String(adc_type));

    // First, enable force operation write
    if (!enableForceOperationWrite()) {
        LOG_ERROR("Failed to enable force operation write");
        return false;
    }

    // Read current Reg0x19 value, clear the bit, then write back
    uint8_t reg_val = readRegister(SW3538_REG_FORCE_OP2 + 1); // Reg0x19 is Reg0x18 + 1
    LOG_DEBUG("Current Reg0x19 value: 0x" + String(reg_val, HEX));

    uint8_t new_reg_val = reg_val & ~(1 << adc_type);
    LOG_DEBUG("New Reg0x19 value: 0x" + String(new_reg_val, HEX));

    if (!writeRegister(SW3538_REG_FORCE_OP2 + 1, new_reg_val)) {
        LOG_ERROR("Failed to write to Reg0x19");
        return false;
    }

    LOG_INFO("Successfully disabled ADC type: " + String(adc_type));
    return true;
}

// Read ADC data according to datasheet (Reg0x41 and Reg0x42)
uint16_t SW3538::readADCData(uint8_t channel) {
    LOG_DEBUG("Reading ADC data for channel: " + String(channel));

    // Select ADC channel - this latches the corresponding data to Reg0x41 and Reg0x42
    // According to datasheet: Writing to this register latches ADC data to Reg0x41 and Reg0x42
    if (!writeRegister(SW3538_REG_ADC_CONFIG, channel)) {
        LOG_ERROR("Failed to select ADC channel: " + String(channel));
        return 0;
    }
    delay(5); // Sufficient delay for ADC conversion and data latching
    LOG_DEBUG("ADC conversion and latching delay completed");

    // Read ADC data registers (0x41 = low 8 bits, 0x42 = high bits)
    uint8_t low_byte = readRegister(SW3538_REG_ADC_DATA_LOW);    // 0x41
    uint8_t high_byte = readRegister(SW3538_REG_ADC_DATA_HIGH);  // 0x42
    LOG_DEBUG("ADC low byte (0x41): 0x" + String(low_byte, HEX));
    LOG_DEBUG("ADC high byte (0x42): 0x" + String(high_byte, HEX));

    // Combine bytes based on data type (according to datasheet)
    uint16_t adc_value = 0;

    if (channel == 11) { // Unit converted output voltage (14-bit)
        // For channel 11: adc_data[14:08] in Reg0x42, adc_data[07:00] in Reg0x41
        // 修正：高字节的低7位对应adc_data[14:08]，而不是6位
        adc_value = ((uint16_t)(high_byte & 0x7F) << 8) | low_byte; // 14-bit resolution
        LOG_DEBUG("14-bit ADC value (channel 11): " + String(adc_value) + " (0x" + String(adc_value, HEX) + ")");
    } else { // Other channels (12-bit)
        // For other channels: adc_data[11:08] in Reg0x42, adc_data[07:00] in Reg0x41
        adc_value = ((uint16_t)(high_byte & 0x0F) << 8) | low_byte; // 12-bit resolution
        LOG_DEBUG("12-bit ADC value (channel " + String(channel) + "): " + String(adc_value) + " (0x" + String(adc_value, HEX) + ")");
    }

    return adc_value;
}

String SW3538::getFastChargeProtocolString(SW3538_FastChargeProtocol protocol) {
    switch (protocol) {
        case SW3538_FC_NONE: return "NONE";
        case SW3538_FC_QC2_0: return "QC2.0";
        case SW3538_FC_QC3_0: return "QC3.0";
        case SW3538_FC_QC3_PLUS: return "QC3+";
        case SW3538_FC_FCP: return "FCP";
        case SW3538_FC_SCP: return "SCP";
        case SW3538_FC_PD_FIX: return "PD-FIX";
        case SW3538_FC_PD_PPS: return "PD-PPS";
        case SW3538_FC_PE1_1: return "PE1.1";
        case SW3538_FC_PE2_0: return "PE2.0";
        case SW3538_FC_VOOC1_0: return "VOOC1.0";
        case SW3538_FC_VOOC4_0: return "VOOC4.0";
        case SW3538_FC_SFCP: return "SFCP";
        case SW3538_FC_AFC: return "AFC";
        case SW3538_FC_TFCP: return "TFCP";
        default: return "UNKNOWN";
    }
}

// Implement readAllData and printAllData in the next step
// Implement settings functions in the next step




bool SW3538::readAllData() {
    LOG_INFO("Reading all SW3538 data");

    // Read Chip Version
    uint8_t versionReg = readRegister(SW3538_REG_VERSION);
    data.chipVersion = versionReg & 0x03; // Bits 1-0
    LOG_DEBUG("Chip Version: " + String(data.chipVersion));

    // Validate chip version - should be reasonable (0-3)
    if (versionReg == 0xFF || versionReg == 0x00) {
        LOG_ERROR("Chip communication issue - all registers returning 0x" + String(versionReg, HEX));
        LOG_ERROR("Check: 1) I2C address 2) Power supply 3) Wiring 4) Pull-up resistors");
        return false;
    }

    // Read System Max Power
    uint8_t powerReg = readRegister(SW3538_REG_MAX_POWER);
    data.maxPowerW = powerReg & 0x7F; // Bits 6-0, 1W/bit
    LOG_DEBUG("System Max Power: " + String(data.maxPowerW) + "W");

    // Validate power - should be reasonable (0-100W)
    if (data.maxPowerW > 100) {
        LOG_WARNING("Suspicious max power value: " + String(data.maxPowerW) + "W");
    }

    // Read Fast Charge Status and Protocol
    uint8_t fc_ind = readRegister(SW3538_REG_FAST_CHARGE_IND);
    
    // If all registers are 0xFF, this is likely a communication issue
    if (fc_ind == 0xFF && versionReg == 0xFF && powerReg == 0xFF) {
        LOG_ERROR("All registers returning 0xFF - I2C communication failure");
        return false;
    }
    
    data.fastChargeStatus = ((fc_ind >> 7) & 0x01) || ((fc_ind >> 6) & 0x01); // Bit 7 (voltage protocol) or Bit 6 (protocol indication)
    data.pdVersion = (fc_ind >> 4) & 0x03; // Bits 5-4
    data.fastChargeProtocol = (SW3538_FastChargeProtocol)(fc_ind & 0x0F); // Bits 3-0
    LOG_DEBUG("Fast Charge Status: " + String(data.fastChargeStatus ? "Active" : "Inactive"));
    LOG_DEBUG("Fast Charge Protocol: " + getFastChargeProtocolString(data.fastChargeProtocol));
    LOG_DEBUG("PD Version: " + String(data.pdVersion));

    // Read System Status 0 (BUCK status)
    uint8_t sys_status0 = readRegister(SW3538_REG_SYS_STATUS0);
    data.path1BuckStatus = (sys_status0 >> 0) & 0x01; // Bit 0
    data.path2BuckStatus = (sys_status0 >> 1) & 0x01; // Bit 1
    LOG_DEBUG("Path 1 BUCK Status: " + String(data.path1BuckStatus ? "Open" : "Closed"));
    LOG_DEBUG("Path 2 BUCK Status: " + String(data.path2BuckStatus ? "Open" : "Closed"));

    // Read System Status 1 (Online status)
    uint8_t sys_status1 = readRegister(SW3538_REG_SYS_STATUS1);
    data.path1Online = (sys_status1 >> 1) & 0x01; // Bit 1
    data.path2Online = (sys_status1 >> 0) & 0x01; // Bit 0
    LOG_DEBUG("Path 1 Online: " + String(data.path1Online ? "Yes" : "No"));
    LOG_DEBUG("Path 2 Online: " + String(data.path2Online ? "Yes" : "No"));

    // Enable ADC for all required measurements
    // Note: VIN ADC is off by default, need to enable Reg0x18[6]
    // According to datasheet, ADC data selection corresponds to:
    // 1: Latch path1 protocol current data idis1
    // 2: Latch path2 protocol current data idis2
    // 5: Latch output voltage data
    // 6: Latch input voltage data
    // 7: Latch NTC channel voltage data
    // 11: Latch unit converted output voltage data
    if (!enableADC(6)) {  // Enable input voltage ADC
        LOG_ERROR("Failed to enable VIN ADC sampling");
        return false;
    }
    if (!enableADC(5)) {  // Enable output voltage ADC
        LOG_ERROR("Failed to enable VOUT ADC sampling");
        return false;
    }
    if (!enableADC(2)) {  // Enable path2 current ADC
        LOG_ERROR("Failed to enable Path 2 Current ADC sampling");
        return false;
    }
    if (!enableADC(1)) {  // Enable path1 current ADC
        LOG_ERROR("Failed to enable Path 1 Current ADC sampling");
        return false;
    }
    if (!enableADC(7)) {  // Enable NTC ADC
        LOG_ERROR("Failed to enable NTC ADC sampling");
        return false;
    }
    LOG_INFO("All required ADC channels enabled");

    // Enable VIN ADC (specifically required as per datasheet)
    uint8_t reg_val = readRegister(0x18); // Read Reg0x18
    reg_val |= (1 << 6); // Set bit 6 to enable VIN ADC
    if (!writeRegister(0x18, reg_val)) {
        LOG_ERROR("Failed to enable VIN ADC via Reg0x18");
        return false;
    }

    // Read Current Data (Path 1 & 2)
    data.currentPath1mA = readADCData(1) * 2.5; // 2.5mA/bit @5mOhm
    data.currentPath2mA = readADCData(2) * 2.5; // 2.5mA/bit @5mOhm
    LOG_DEBUG("Path 1 Current: " + String(data.currentPath1mA) + "mA");
    LOG_DEBUG("Path 2 Current: " + String(data.currentPath2mA) + "mA");

    // Read Voltage Data
    data.inputVoltagemV = readADCData(6) * 10; // 10mV/bit
    data.outputVoltagemV = readADCData(11) * 1; // 1mV/bit (unit converted)
    LOG_DEBUG("Input Voltage: " + String(data.inputVoltagemV) + "mV");
    LOG_DEBUG("Output Voltage: " + String(data.outputVoltagemV) + "mV");

    // Read NTC Temperature
    uint16_t ntc_voltage_adc = readADCData(7); // 1.2mV/bit
    uint8_t ntc_current_state = readRegister(SW3538_REG_NTC_CURRENT_STATE) >> 7; // Bit 7: 0=20uA, 1=40uA
    float ntc_voltage_mv = ntc_voltage_adc * 1.2; // Convert ADC value to voltage (1.2mV/bit)
    LOG_DEBUG("NTC Voltage ADC: " + String(ntc_voltage_adc) + " (" + String(ntc_voltage_mv) + "mV)");
    LOG_DEBUG("NTC Current State: " + String(ntc_current_state) + " (" + (ntc_current_state ? "40uA" : "20uA") + ")");

    // Calculate NTC resistance
    float ntc_current_ua = ntc_current_state ? 40.0f : 20.0f; // Current in uA
    float ntc_resistance_kohm = (ntc_voltage_mv / ntc_current_ua); // R = V/I (V in mV, I in uA gives R in kOhm)
    LOG_DEBUG("NTC Resistance: " + String(ntc_resistance_kohm) + "kOhm");

    // Simplified NTC temperature calculation (using typical B-value of 3950 for 10k NTC)
    // This is a basic implementation - for more accuracy, use a lookup table or full Steinhart-Hart equation
    float ref_temp = 25.0f; // Reference temperature (°C)
    float ref_resistance_kohm = 10.0f; // Reference resistance at 25°C (kOhm)
    float b_value = 3950.0f; // B-value of the NTC thermistor

    float temp_k = 1.0f / (1.0f / (ref_temp + 273.15f) + (1.0f / b_value) * log(ntc_resistance_kohm / ref_resistance_kohm));
    data.ntcTemperatureC = temp_k - 273.15f;

    // Validate temperature (should be within reasonable range)
    if (data.ntcTemperatureC < -40 || data.ntcTemperatureC > 125) {
        LOG_WARNING("NTC temperature out of range: " + String(data.ntcTemperatureC) + "°C");
        data.ntcTemperatureC = -999; // Mark as invalid
    } else {
        LOG_DEBUG("NTC Temperature: " + String(data.ntcTemperatureC) + "°C");
    }

    // Disable ADCs after reading
    if (!disableADC(6)) {
        LOG_ERROR("Failed to disable VIN ADC sampling");
        return false;
    }
    if (!disableADC(5)) {
        LOG_ERROR("Failed to disable VOUT ADC sampling");
        return false;
    }
    if (!disableADC(2)) {
        LOG_ERROR("Failed to disable Path 2 Current ADC sampling");
        return false;
    }
    if (!disableADC(1)) {
        LOG_ERROR("Failed to disable Path 1 Current ADC sampling");
        return false;
    }
    if (!disableADC(7)) {
        LOG_ERROR("Failed to disable NTC ADC sampling");
        return false;
    }
    LOG_INFO("All ADC channels disabled after reading");

    LOG_INFO("Successfully read all SW3538 data");
    return true;
}

void SW3538::printAllData(Print& serial) {
    serial.println("--- SW3538 Data ---");
    serial.print("Chip Version: "); serial.println(data.chipVersion);
    serial.print("System Max Power: "); serial.print(data.maxPowerW); serial.println("W");
    serial.print("Fast Charge Status: "); serial.println(data.fastChargeStatus ? "Active" : "Inactive");
    serial.print("Fast Charge Protocol: "); serial.println(getFastChargeProtocolString(data.fastChargeProtocol));
    serial.print("PD Version: ");
    if (data.pdVersion == 1) serial.println("PD2.0");
    else if (data.pdVersion == 2) serial.println("PD3.0");
    else serial.println("Reserved");
    serial.print("Path 1 Online: "); serial.println(data.path1Online ? "Yes" : "No");
    serial.print("Path 2 Online: "); serial.println(data.path2Online ? "Yes" : "No");
    serial.print("Path 1 BUCK Status: "); serial.println(data.path1BuckStatus ? "Open" : "Closed");
    serial.print("Path 2 BUCK Status: "); serial.println(data.path2BuckStatus ? "Open" : "Closed");
    serial.print("Path 1 Current: "); serial.print(data.currentPath1mA); serial.println("mA");
    serial.print("Path 2 Current: "); serial.print(data.currentPath2mA); serial.println("mA");
    serial.print("Input Voltage: "); serial.print(data.inputVoltagemV); serial.println("mV");
    serial.print("Output Voltage: "); serial.print(data.outputVoltagemV); serial.println("mV");
    serial.print("NTC Temperature: ");
    if (data.ntcTemperatureC == -999) serial.println("N/A (NTC curve needed)");
    else serial.print(data.ntcTemperatureC); serial.println("C");
    serial.println("-------------------");
}

// Settings functions
bool SW3538::setNTC(uint8_t current_state) {
    LOG_INFO("Setting NTC current state: " + String(current_state));

    if (current_state > 1) {
        LOG_ERROR("Invalid NTC current state (must be 0 or 1)");
        return false; // Only 0 (20uA) or 1 (40uA)
    }

    // Enable I2C write
    if (!enableI2CWrite()) {
        LOG_ERROR("Failed to enable I2C write");
        return false;
    }

    // Read current Reg0x44, modify bit 7, then write back
    uint8_t reg_val = readRegister(SW3538_REG_NTC_CURRENT_STATE);
    LOG_DEBUG("Current Reg0x44 value: 0x" + String(reg_val, HEX));

    uint8_t new_reg_val = (reg_val & 0x7F) | (current_state << 7); // Clear bit 7, then set it
    LOG_DEBUG("New Reg0x44 value: 0x" + String(new_reg_val, HEX));

    bool result = writeRegister(SW3538_REG_NTC_CURRENT_STATE, new_reg_val);
    if (result) {
        LOG_INFO("Successfully set NTC current state");
    } else {
        LOG_ERROR("Failed to set NTC current state");
    }
    return result;
}

bool SW3538::setMOSInternalResistance(uint8_t mos_setting) {
    if (mos_setting > 3) return false; // Only 0, 1, 2, 3
    // Enable I2C write
    if (!enableI2CWrite()) return false;
    // Read current Reg0x107, modify bits 7-6, then write back
    uint8_t reg_val = readRegister(SW3538_REG_MOS_SETTING);
    reg_val = (reg_val & 0x3F) | (mos_setting << 6); // Clear bits 7-6, then set them
    return writeRegister(SW3538_REG_MOS_SETTING, reg_val);
}

bool SW3538::setNTCOverTempThreshold(uint8_t threshold_setting) {
    // 0: 65C, 1: 75C, 2: 85C, 3: 95C, 4: 105C, 5: 115C, 6: 125C, 7: Disable
    if (threshold_setting > 7) return false;
    // Enable I2C write
    if (!enableI2CWrite()) return false;
    // Read current Reg0x10D, modify bits 5-3, then write back
    uint8_t reg_val = readRegister(SW3538_REG_TEMP_SETTING);
    reg_val = (reg_val & 0xC7) | (threshold_setting << 3); // Clear bits 5-3, then set them
    return writeRegister(SW3538_REG_TEMP_SETTING, reg_val);
}


