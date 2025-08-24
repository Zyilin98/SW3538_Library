#ifndef SW3538_H
#define SW3538_H

#include <Arduino.h>
#include <Wire.h> // For hardware I2C

// Define SW3538 Registers
#define SW3538_DEFAULT_ADDRESS     0x3C  // Default I2C address (0x3C or 0x3D depending on A0 pin)
#define SW3538_REG_VERSION          0x00
#define SW3538_REG_MAX_POWER        0x02
#define SW3538_REG_FAST_CHARGE_IND  0x09
#define SW3538_REG_SYS_STATUS0      0x0A
#define SW3538_REG_SYS_STATUS1      0x0D
#define SW3538_REG_I2C_ENABLE       0x10
#define SW3538_REG_FORCE_OP_ENABLE  0x15
#define SW3538_REG_FORCE_OP2        0x18
#define SW3538_REG_ADC_CONFIG       0x40
#define SW3538_REG_ADC_DATA_LOW     0x41
#define SW3538_REG_ADC_DATA_HIGH    0x42
#define SW3538_REG_NTC_CURRENT_STATE 0x44
#define SW3538_REG_MOS_SETTING      0x107
#define SW3538_REG_TEMP_SETTING     0x10D

// Fast Charge Protocol Definitions (from Reg0x09, bits 3-0)
enum SW3538_FastChargeProtocol {
    SW3538_FC_NONE = 0,
    SW3538_FC_QC2_0 = 1,
    SW3538_FC_QC3_0 = 2,
    SW3538_FC_QC3_PLUS = 3,
    SW3538_FC_FCP = 4,
    SW3538_FC_SCP = 5,
    SW3538_FC_PD_FIX = 6,
    SW3538_FC_PD_PPS = 7,
    SW3538_FC_PE1_1 = 8,
    SW3538_FC_PE2_0 = 9,
    SW3538_FC_VOOC1_0 = 10,
    SW3538_FC_VOOC4_0 = 11,
    SW3538_FC_SFCP = 13,
    SW3538_FC_AFC = 14,
    SW3538_FC_TFCP = 15
};

// Structure to hold all chip data
typedef struct {
    uint8_t chipVersion;
    uint16_t maxPowerW; // in Watts
    bool fastChargeStatus;
    SW3538_FastChargeProtocol fastChargeProtocol;
    uint8_t pdVersion; // 0: Reserved, 1: PD2.0, 2: PD3.0
    bool path1Online;
    bool path2Online;
    bool path1BuckStatus;
    bool path2BuckStatus;
    int16_t currentPath1mA; // in mA
    int16_t currentPath2mA; // in mA
    uint16_t inputVoltagemV; // in mV
    uint16_t outputVoltagemV; // in mV
    int16_t ntcTemperatureC; // in Celsius
} SW3538_Data_t;

class SW3538 {
public:
    // Constructor for hardware I2C with default address
    SW3538(uint8_t address = SW3538_DEFAULT_ADDRESS);

    // Constructor for software I2C (if needed, not implemented by default Wire library)
    // SW3538(uint8_t address, uint8_t sdaPin, uint8_t sclPin);

    void begin();
    bool testI2CAddress(uint8_t address); // Test specific I2C address
    void scanI2CAddresses(); // Scan all possible I2C addresses
    bool readAllData();
    void printAllData(Print& serial);

    // Settings functions
    bool setNTC(uint8_t current_state); // 0: 20uA, 1: 40uA
    bool setMOSInternalResistance(uint8_t mos_setting); // 0: 2mOhm, 1: 4mOhm, 2: 16mOhm, 3: 8mOhm
    bool setNTCOverTempThreshold(uint8_t threshold_setting); // 0: 65C, ..., 4: 105C, ..., 7: Disable

    String getFastChargeProtocolString(SW3538_FastChargeProtocol protocol);

    SW3538_Data_t data; // Public member to access read data

private:
    uint8_t _address;
    // uint8_t _sdaPin; // For software I2C
    // uint8_t _sclPin; // For software I2C

    uint8_t readRegister(uint16_t reg);
    bool writeRegister(uint16_t reg, uint8_t value);
    bool enableI2CWrite();
    bool enableForceOperationWrite();
    bool enableADC(uint8_t adc_type);
    bool disableADC(uint8_t adc_type);
    uint16_t readADCData(uint8_t channel);
};

#endif


