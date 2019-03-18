/*
  DS3234.h - Arduino library support for the DS3234 SPI Bus RTC
  Copyright (C)2014 Henning Karlsen. All right reserved
  
  This library has been made to easily interface and use the DS3234 RTC with the 
  Arduino and chipKit development boards. 
  This library makes use of the built-in hardware SPI port of the microcontroller 
  so there are some pin connections that are required (see the manual).

  You can find the latest version of the library at
	http://www.henningkarlsen.com/electronics

  If you make any modifications or improvements to the code, I would appreciate
  that you share the code with me so that I might include it in the next release.
  I can be contacted through http://www.henningkarlsen.com/electronics/contact.php

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.
*/
#ifndef DS3234_h
#define DS3234_h
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
#include "driverlib/timer.h"


#define FORMAT_SHORT	1
#define FORMAT_LONG		2

#define FORMAT_LITTLEENDIAN	1
#define FORMAT_BIGENDIAN	2
#define FORMAT_MIDDLEENDIAN	3

#define MONDAY		1
#define TUESDAY		2
#define WEDNESDAY	3
#define THURSDAY	4
#define FRIDAY		5
#define SATURDAY	6
#define SUNDAY		7

#define SYSCLOCKFREQ 80000000

class Time
{
public:
	uint8_t		hour;
	uint8_t		min;
	uint8_t		sec;
	uint8_t		date;
	uint8_t		mon;
	uint16_t	year;
	uint8_t		dow;

	Time();
};

class DS3234
{
public:
	DS3234(uint8_t ss_pin);
	void	begin();
	Time	getTime();
	void	setTime(uint8_t hour, uint8_t min, uint8_t sec);
	void	setDate(uint8_t date, uint8_t mon, uint16_t year);
	void	setDOW(uint8_t dow);

	char	*getTimeStr(uint8_t format=FORMAT_LONG);
	char	*getDateStr(uint8_t slformat=FORMAT_LONG, uint8_t eformat=FORMAT_LITTLEENDIAN, char divider='.');
	char	*getDOWStr(uint8_t format=FORMAT_LONG);
	char	*getMonthStr(uint8_t format=FORMAT_LONG);

	float	getTemp();

	void	poke(uint8_t addr, uint8_t value);
	uint8_t	peek(uint8_t addr);

private:
	uint8_t _ss_pin;
	uint8_t _burstArray[8];

	uint8_t	_readByte();
	void	_writeByte(uint8_t value);
	uint8_t	_readRegister(uint8_t reg);
	void 	_writeRegister(uint8_t reg, uint8_t value);
	void	_burstRead();
	void    _ssPinHigh();
	void    _ssPinLow();
	uint8_t	_decode(uint8_t value);
	uint8_t	_decodeH(uint8_t value);
	uint8_t	_decodeY(uint8_t value);
	uint8_t	_encode(uint8_t vaule);

	void	_SPIstart();
	void	_SPIwrite(uint8_t data);
	uint8_t	_SPIread(void);

};
#endif
