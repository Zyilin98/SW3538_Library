# SW3538 Library

Arduino library for SW3538/SW3556/SW3558

## Supported Chips

- SW3538
- SW3556
- SW3558


## Quick Start

```cpp
#include "SW3538.h"

// Basic usage with default I2C pins
SW3538 sw3538(0x3C);

// Custom I2C pins for ESP32
SW3538 sw3538(0x3C, 21, 22);

void setup() {
    Serial.begin(115200);
    sw3538.begin();
}

void loop() {
    if (sw3538.readAllData()) {
        float current = sw3538.getCurrent();  // mA
        bool path1 = sw3538.isPath1Connected();
        bool path2 = sw3538.isPath2Connected();
        
        Serial.print("Current: ");
        Serial.print(current);
        Serial.println("mA");
    }
    delay(100);
}
```

## API Reference

### Constructor
```cpp
SW3538(uint8_t address = 0x3C);
SW3538(uint8_t address, int sdaPin, int sclPin);
```

### Methods
```cpp
bool begin();              // Initialize I2C
bool readAllData();        // Read all registers
float getCurrent();        // Total current (mA)
float getVoltage();        // Output voltage (V)
bool isFastCharge();       // Fast charge active
bool isPath1Connected();   // Device on path 1
bool isPath2Connected();   // Device on path 2
```

## Wiring

| SW3538 Pin | Arduino Pin |
|------------|-------------|
| SDA        | A4 / SDA    |
| SCL        | A5 / SCL    |
| INT        | 2 (optional)|

## License

MIT License - see LICENSE file for details.