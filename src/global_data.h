#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include "SW3538.h"

// Global variable to store SW3538 data
extern SW3538_Data_t sw3538Data;

// Function to get SW3538 data
const SW3538_Data_t& getSW3538Data();

// Function to check if SW3538 data is valid
bool isSW3538DataValid();

#endif