/*
 * HMC39XX.h
 *
 *  Created on: 01.12.2017
 *      Author: Raphy
 */

#ifndef HCMS39XX_H_
#define HCMS39XX_H_
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_timer.h"
#include "inc/hw_sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
//#include "ASCII_Char_Set.h"
#define DISPLAY_NUMBER 14
#define PEAK_PIX_CURR 0x1
#define SYSCLOCKFREQ 80000000

class HCMS39XX {
public:
    HCMS39XX();
    void initialize();
    void displayTest();
    uint8_t* encodeChar(char c);
    void write4Chars(uint8_t text[4]);
    void setBrightness(uint8_t level);
    void setControlWord0(uint8_t peak_curr, bool sleep, uint8_t brightness);
    void setControlWord1(bool doutSync, bool extOsziPresc);
    void selectDotRegister();
    void selectControlRegister();
    void chipEnable();
    void chipDisable();
    void clkLow();
    void clkHigh();
    void blankDisplay();
    void unBlankDisplay();
    void writeChar(uint8_t c);
    void writeString(char str[]);
    void setupGPIO();
    void waitNoTicks(int no);
private:

};



#endif /* HMC39XX_H_ */
