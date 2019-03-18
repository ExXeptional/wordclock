/*
  DS3234.cpp - Arduino library support for the DS3234 SPI Bus RTC
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
#include "DS3234.h"


#define REG_SEC			0x00
#define REG_MIN			0x01
#define REG_HOUR		0x02
#define REG_DOW			0x03
#define REG_DATE		0x04
#define REG_MON			0x05
#define REG_YEAR		0x06
#define REG_CONTROL		0x0E
#define REG_STATUS		0x0F
#define REG_TEMP_MSB	0x11
#define REG_TEMP_LSB	0x12
#define REG_RAM_ADDR	0x18
#define REG_RAM_DATA	0x19

/* Public */

Time::Time()
{
	this->year = 2014;
	this->mon  = 1;
	this->date = 1;
	this->hour = 0;
	this->min  = 0;
	this->sec  = 0;
	this->dow  = 3;
}

DS3234::DS3234(uint8_t ss_pin)
{
	_ss_pin = ss_pin;
}

void DS3234::begin()
{
	_SPIstart();
	_writeRegister(REG_CONTROL, 0x00);
	_writeRegister(REG_STATUS, 0x00);
}

Time DS3234::getTime()
{
	Time t;
	_burstRead();
	t.sec	= _decode(_burstArray[0]);
	t.min	= _decode(_burstArray[1]);
	t.hour	= _decodeH(_burstArray[2]);
	t.dow	= _burstArray[3];
	t.date	= _decode(_burstArray[4]);
	t.mon	= _decode(_burstArray[5]);
	t.year	= _decodeY(_burstArray[6])+2000;
	return t;
}

void DS3234::setTime(uint8_t hour, uint8_t min, uint8_t sec)
{
	if ((hour<24) && (min<60) && (sec<60))
	{
		_writeRegister(REG_HOUR, _encode(hour));
		_writeRegister(REG_MIN, _encode(min));
		_writeRegister(REG_SEC, _encode(sec));
	}
}

void DS3234::setDate(uint8_t date, uint8_t mon, uint16_t year)
{
	if (((date>0) && (date<=31)) && ((mon>0) && (mon<=12)) && ((year>=2000) && (year<2100)))
	{
		year -= 2000;
		_writeRegister(REG_YEAR, _encode(year));
		_writeRegister(REG_MON, _encode(mon));
		_writeRegister(REG_DATE, _encode(date));
	}
}

void DS3234::setDOW(uint8_t dow)
{
	if ((dow>0) && (dow<8))
		_writeRegister(REG_DOW, dow);
}

char *DS3234::getTimeStr(uint8_t format)
{
//	char *output= "xxxxxxxx";
	static char output[] = "xxxxxxxx";
	Time t;
	t=getTime();
	if (t.hour<10)
		output[0]=48;
	else
		output[0]=char((t.hour / 10)+48);
	output[1]=char((t.hour % 10)+48);
	output[2]=58;
	if (t.min<10)
		output[3]=48;
	else
		output[3]=char((t.min / 10)+48);
	output[4]=char((t.min % 10)+48);
	output[5]=58;
	if (format==FORMAT_SHORT)
		output[5]=0;
	else
	{
	if (t.sec<10)
		output[6]=48;
	else
		output[6]=char((t.sec / 10)+48);
	output[7]=char((t.sec % 10)+48);
	output[8]=0;
	}
//	return output;
	return (char*)&output;
}

char *DS3234::getDateStr(uint8_t slformat, uint8_t eformat, char divider)
{
//	char *output= "xxxxxxxxxx";
	static char output[] = "xxxxxxxxxx";
	int yr, offset;
	Time t;
	t=getTime();
	switch (eformat)
	{
		case FORMAT_LITTLEENDIAN:
			if (t.date<10)
				output[0]=48;
			else
				output[0]=char((t.date / 10)+48);
			output[1]=char((t.date % 10)+48);
			output[2]=divider;
			if (t.mon<10)
				output[3]=48;
			else
				output[3]=char((t.mon / 10)+48);
			output[4]=char((t.mon % 10)+48);
			output[5]=divider;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-2000;
				if (yr<10)
					output[6]=48;
				else
					output[6]=char((yr / 10)+48);
				output[7]=char((yr % 10)+48);
				output[8]=0;
			}
			else
			{
				yr=t.year;
				output[6]=char((yr / 1000)+48);
				output[7]=char(((yr % 1000) / 100)+48);
				output[8]=char(((yr % 100) / 10)+48);
				output[9]=char((yr % 10)+48);
				output[10]=0;
			}
			break;
		case FORMAT_BIGENDIAN:
			if (slformat==FORMAT_SHORT)
				offset=0;
			else
				offset=2;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-2000;
				if (yr<10)
					output[0]=48;
				else
					output[0]=char((yr / 10)+48);
				output[1]=char((yr % 10)+48);
				output[2]=divider;
			}
			else
			{
				yr=t.year;
				output[0]=char((yr / 1000)+48);
				output[1]=char(((yr % 1000) / 100)+48);
				output[2]=char(((yr % 100) / 10)+48);
				output[3]=char((yr % 10)+48);
				output[4]=divider;
			}
			if (t.mon<10)
				output[3+offset]=48;
			else
				output[3+offset]=char((t.mon / 10)+48);
			output[4+offset]=char((t.mon % 10)+48);
			output[5+offset]=divider;
			if (t.date<10)
				output[6+offset]=48;
			else
				output[6+offset]=char((t.date / 10)+48);
			output[7+offset]=char((t.date % 10)+48);
			output[8+offset]=0;
			break;
		case FORMAT_MIDDLEENDIAN:
			if (t.mon<10)
				output[0]=48;
			else
				output[0]=char((t.mon / 10)+48);
			output[1]=char((t.mon % 10)+48);
			output[2]=divider;
			if (t.date<10)
				output[3]=48;
			else
				output[3]=char((t.date / 10)+48);
			output[4]=char((t.date % 10)+48);
			output[5]=divider;
			if (slformat==FORMAT_SHORT)
			{
				yr=t.year-2000;
				if (yr<10)
					output[6]=48;
				else
					output[6]=char((yr / 10)+48);
				output[7]=char((yr % 10)+48);
				output[8]=0;
			}
			else
			{
				yr=t.year;
				output[6]=char((yr / 1000)+48);
				output[7]=char(((yr % 1000) / 100)+48);
				output[8]=char(((yr % 100) / 10)+48);
				output[9]=char((yr % 10)+48);
				output[10]=0;
			}
			break;
	}
//	return output;
	return (char*)&output;
}

char *DS3234::getDOWStr(uint8_t format)
{
	char *output= "xxxxxxxxx";
	Time t;
	t=getTime();
	switch (t.dow)
	{
		case MONDAY:
			output="Monday";
			break;
		case TUESDAY:
			output="Tuesday";
			break;
		case WEDNESDAY:
			output="Wednesday";
			break;
		case THURSDAY:
			output="Thursday";
			break;
		case FRIDAY:
			output="Friday";
			break;
		case SATURDAY:
			output="Saturday";
			break;
		case SUNDAY:
			output="Sunday";
			break;
	}     
	if (format==FORMAT_SHORT)
		output[3]=0;
	return output;
}

char *DS3234::getMonthStr(uint8_t format)
{
	char *output= "xxxxxxxxx";
	Time t;
	t=getTime();
	switch (t.mon)
	{
		case 1:
			output="January";
			break;
		case 2:
			output="February";
			break;
		case 3:
			output="March";
			break;
		case 4:
			output="April";
			break;
		case 5:
			output="May";
			break;
		case 6:
			output="June";
			break;
		case 7:
			output="July";
			break;
		case 8:
			output="August";
			break;
		case 9:
			output="September";
			break;
		case 10:
			output="October";
			break;
		case 11:
			output="November";
			break;
		case 12:
			output="December";
			break;
	}     
	if (format==FORMAT_SHORT)
		output[3]=0;
	return output;
}

float DS3234::getTemp()
{
	float   temp = 0;
	uint8_t msb, lsb;

	msb = _readRegister(REG_TEMP_MSB);
	lsb = _readRegister(REG_TEMP_LSB);

	temp = (msb & 0x7f) + (((lsb & 0xc0)>>6)*0.25);

	return (msb & 0x80)?-temp:temp;
}

void DS3234::poke(uint8_t addr, uint8_t value)
{
    _ssPinLow();
	_writeByte(REG_RAM_ADDR + 0x80);
	_writeByte(addr);
	_writeByte(value);
	_ssPinHigh();
}

uint8_t DS3234::peek(uint8_t addr)
{
	uint8_t readValue;

	_ssPinLow();
	_writeByte(REG_RAM_ADDR + 0x80);
	_writeByte(addr);
	_ssPinHigh();
	_ssPinLow();
	_writeByte(REG_RAM_DATA);
	readValue = _readByte();
	_ssPinHigh();

	return readValue;
}

/* Private */

uint8_t DS3234::_readByte()
{
	return _SPIread();
}

void DS3234::_writeByte(uint8_t value)
{
	_SPIwrite(value);
}

uint8_t DS3234::_readRegister(uint8_t reg)
{
	uint8_t readValue;

	_ssPinLow();
	_writeByte(reg);
	readValue = _readByte();
	_ssPinHigh();
	return readValue;
}

void DS3234::_writeRegister(uint8_t reg, uint8_t value)
{
    _ssPinLow();
	_writeByte(reg + 0x80);
	_writeByte(value);
	_ssPinHigh();
}

void DS3234::_burstRead()
{
    _ssPinLow();
	_writeByte(0);
	for (int i=0; i<8; i++)
	{
		_burstArray[i] = _readByte();
	}
	_ssPinHigh();

}

uint8_t	DS3234::_decode(uint8_t value)
{
	uint8_t decoded = value & 127;
	decoded = (decoded & 0x0F) + (10 * ((decoded & 0x70) >> 4));
	return decoded;
}

uint8_t DS3234::_decodeH(uint8_t value)
{
  if (value & 128)
    value = (value & 15) + (12 * ((value & 32) >> 5));
  else
    value = (value & 15) + (10 * ((value & 48) >> 4));
  return value;
}

uint8_t	DS3234::_decodeY(uint8_t value)
{
	uint8_t decoded = (value & 0x0F) + (10 * ((value & 0xF0)>>4));
	return decoded;
}

uint8_t DS3234::_encode(uint8_t value)
{
	uint8_t encoded = ((value / 10) << 4) + (value % 10);
	return encoded;
}

void DS3234::_SPIstart()
{

    /*
     * PINS:
     * SCLK : PB4
     * MOSI : PB7
     * MISO : PB6
     * SS   : PB5
     *
     */
    uint32_t pui32DataRx[255];
   // The SSI2 peripheral must be enabled for use.
   //
   SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);


   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
   GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE,GPIO_PIN_0);

   GPIOPinConfigure(GPIO_PB4_SSI2CLK);
   //GPIOPinConfigure(GPIO_PB5_SSI2FSS);
   GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_5);
   GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x20);
   GPIOPinConfigure(GPIO_PB6_SSI2RX);
   GPIOPinConfigure(GPIO_PB7_SSI2TX);

   GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_7 | GPIO_PIN_6 /*| GPIO_PIN_5*/ |GPIO_PIN_4);

   SSIConfigSetExpClk(SSI2_BASE, SYSCLOCKFREQ, SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 200000, 8);

   SSIEnable(SSI2_BASE);

   _ssPinLow();

   while(SSIDataGetNonBlocking(SSI2_BASE, &pui32DataRx[0]))
   {
   }
   _ssPinHigh();

}

void DS3234::_SPIwrite(uint8_t data)
{
    uint32_t dummy;
    SSIDataPut(SSI2_BASE, data);
    while(SSIBusy(SSI2_BASE))
    {
    }
    SSIDataGet(SSI2_BASE, &dummy);
    while(SSIBusy(SSI2_BASE))
    {
    }
}

uint8_t DS3234::_SPIread(void)
{
    uint32_t data;
    SSIDataPut(SSI2_BASE, 0xff);
    while(SSIBusy(SSI2_BASE))
    {
    }
    SSIDataGet(SSI2_BASE, &data);
    while(SSIBusy(SSI2_BASE))
    {
    }
    return (uint8_t)(data & 0xff);
}

void DS3234::_ssPinHigh()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x20);
}

void DS3234::_ssPinLow()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_5, 0x0);
}
