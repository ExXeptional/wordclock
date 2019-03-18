# wordclock

## What is this?

This is a project of mine in which I built a clock from a Tiva Launchpad, 5x7 char led displays, a light sensor and a RTC.

## Display driver:

I used  HCMS39xx family displays, the consist of 8x 5x7 char fields. 
The display driver is contained in the files HCMS39XX.cpp/.h and ASCII_Char_Set.h which specifies the font.

## RTC driver:

For time keeping I used a separate real time clock IC with calendar functionality.
The RTC driver for the SPI interface is contained in the files DS3234.cpp/.h

## Light sensor:

I used a ISL29023 light sensor, I control it with the functions in the files ISL29023_Interface.cpp/.h
The interface was adapted from this repository: https://github.com/madvoid/Tiva-Test/tree/master/ISL29023

## Notice

This program is not really well written, most of the hardware initialization happens in the main.cpp but some in the other classes.
If you want to use parts of this in your project, feel free to do so. If you have some questions, please ask.
