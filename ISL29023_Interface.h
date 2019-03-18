/*
 * ISL29023_Interface.h
 * Original By Nipun Gunawardena
 * https://github.com/madvoid/Tiva-Test/tree/master/ISL29023
 *
 * Adapted by Raphael Kriegl
 *  Created on: 24.05.2018
 */

#ifndef ISL29023_INTERFACE_H_
#define ISL29023_INTERFACE_H_
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
#include "inc/hw_ssi.h"
#include "inc/hw_adc.h"
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
#include "driverlib/ssi.h"
#include "driverlib/i2c.h"

#define ISL29023_I2C_ADDRESS 0x44
#define ISL29023_REG_COMMANDI 0x00
#define ISL29023_REG_COMMANDII 0x01
#define ISL29023_REG_DATALSB 0x02
#define ISL29023_REG_DATAMSB 0x03
#define ISL29023_REG_LOWINTLSB 0x04
#define ISL29023_REG_LOWINTMSB 0x05
#define ISL29023_REG_UPINTLSB 0x06
#define ISL29023_REG_UPINTMSB 0x07

#define ISL29023_COMMANDI_PERSIST1 0x00
#define ISL29023_COMMANDI_PERSIST4 0x01
#define ISL29023_COMMANDI_PERSIST8 0x02
#define ISL29023_COMMANDI_PERSIST16 0x03
#define ISL29023_COMMANDI_NOPOW 0x00
#define ISL29023_COMMANDI_ONEALS 0x20
#define ISL29023_COMMANDI_ONEIR 0x40
#define ISL29023_COMMANDI_CONTALS 0xA0
#define ISL29023_COMMANDI_CONTIR 0xC0

#define ISL29023_COMMANDII_RANGE1k 0x00
#define ISL29023_COMMANDII_RANGE4k 0x02
#define ISL29023_COMMANDII_RANGE16k 0x01
#define ISL29023_COMMANDII_RANGE64k 0x03
#define ISL29023_COMMANDII_RES16 0x00
#define ISL29023_COMMANDII_RES12 0x04
#define ISL29023_COMMANDII_RES8 0x08
#define ISL29023_COMMANDII_RES4 0xC

#define SYSCLOCKFREQ 80000000

class Light_Interface{

private:

   uint32_t resSetting;
   uint16_t rangeSetting;
   uint32_t rawVals[2];
   float alpha;
   float beta; // TI Code has following four possible values: 95.238, 23.810, 5.952, 1.486 - all based on range
   float alsVal;
   float irVal;

   void ISL29023ChangeSettings(uint8_t range, uint8_t resolution);
   void ISL29023GetRawALS();
   void ISL29023GetALS();
   void ISL29023GetRawIR();
   void ISL29023GetIR();

   void configureI2C();

public:
   void initialize();
   float getLux();

};



#endif /* ISL29023_INTERFACE_H_ */
