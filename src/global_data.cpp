#include "global_data.h"

// Global variable definition
SW3538_Data_t sw3538Data;

// Function to get SW3538 data
const SW3538_Data_t& getSW3538Data() {
    return sw3538Data;
}

// Function to check if SW3538 data is valid
bool isSW3538DataValid() {
    return (sw3538Data.chipVersion <= 3) && (sw3538Data.maxPowerW <= 100);
}