/*
 * ISL29023_Interface.cpp
 *
 *  Created on: 24.05.2018
 *      Author: Raphy
 */

#include "ISL29023_Interface.h"


void Light_Interface::configureI2C()
{
    // Enable peripherals used by I2C
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //For Interrupt Pin
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    // Setup GPIO
    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);
    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_4);

    //Configure Pullup for External OpenDrain /INT
    GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    //Configure Interrupt - not needed
    //GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);
    //GPIOPinIntEnable(GPIO_PORTE_BASE, GPIO_PIN_4);

    // Set GPIO D0 and D1 as SCL and SDA
    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    // Initialize as master - 'true' for fastmode, 'false' for regular
    I2CMasterInitExpClk(I2C1_BASE, SYSCLOCKFREQ, true);
}

void Light_Interface::initialize()
{
    configureI2C();
    ISL29023ChangeSettings(ISL29023_COMMANDII_RANGE64k, ISL29023_COMMANDII_RES16);

}

float Light_Interface::getLux()
{
    Light_Interface::ISL29023GetALS();
    return alsVal;
}


void Light_Interface::ISL29023ChangeSettings(uint8_t range, uint8_t resolution){

    // The input range and resolution should have defines from above passed into them
    // Example: ISL29023ChangeSettings(ISL29023_COMMANDII_RES16, ISL29023_COMMANDII_RANGE64k);
    // Must be called before starting measurements

    // Configure to write, send control register
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, false);
    I2CMasterDataPut(I2C1_BASE, ISL29023_REG_COMMANDII);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Send data byte
    I2CMasterDataPut(I2C1_BASE, (range | resolution));
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Change resSetting in structure
    switch(resolution){
        case ISL29023_COMMANDII_RES16:
            this->resSetting = 65536;
            break;
        case ISL29023_COMMANDII_RES12:
            this->resSetting = 4096;
            break;
        case ISL29023_COMMANDII_RES8:
            this->resSetting = 256;
            break;
        case ISL29023_COMMANDII_RES4:
            this->resSetting = 16;
            break;
        default:
            break;
    }

    switch(range){
        case ISL29023_COMMANDII_RANGE64k:
            this->rangeSetting = 64000;
            break;
        case ISL29023_COMMANDII_RANGE16k:
            this->rangeSetting = 16000;
            break;
        case ISL29023_COMMANDII_RANGE4k:
            this->rangeSetting = 4000;
            break;
        case ISL29023_COMMANDII_RANGE1k:
            this->rangeSetting = 1000;
            break;
        default:
            break;
    }

    this->alpha = (float)this->rangeSetting / (float)this->resSetting;
    this->beta = 95.238;
}

void Light_Interface::ISL29023GetRawALS(){

    // Configure to write, send control register
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, false);
    I2CMasterDataPut(I2C1_BASE, ISL29023_REG_COMMANDI);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Send data byte
    I2CMasterDataPut(I2C1_BASE, ISL29023_COMMANDI_ONEALS | ISL29023_COMMANDI_PERSIST8);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Wait for measurement to complete
    switch(this->resSetting){
        case 65536:
            SysCtlDelay(SYSCLOCKFREQ/3/11);
            break;
        case 4096:
            SysCtlDelay(SYSCLOCKFREQ/3/166);
            break;
        case 256:
            SysCtlDelay(SYSCLOCKFREQ/3/250);
            break;
        case 16:
            SysCtlDelay(SYSCLOCKFREQ/3/250);
            break;
        default:
            SysCtlDelay(SYSCLOCKFREQ/3/11);
            break;
    }

    // Send start, configure to write, send LSB register
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, false);
    I2CMasterDataPut(I2C1_BASE, ISL29023_REG_DATALSB);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Send restart, configure to read
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, true);

    // Read LSB
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(I2C1_BASE)){}
    this->rawVals[1] = I2CMasterDataGet(I2C1_BASE);

    // Read MSB
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(I2CMasterBusy(I2C1_BASE)){}
    this->rawVals[0] = I2CMasterDataGet(I2C1_BASE);
}

void Light_Interface::ISL29023GetALS(){
    ISL29023GetRawALS();
    this->alsVal = this->alpha * ((float)((this->rawVals[0] << 8) | this->rawVals[1]));
}

void Light_Interface::ISL29023GetRawIR(){

    // Configure to write, send control register
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, false);
    I2CMasterDataPut(I2C1_BASE, ISL29023_REG_COMMANDI);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Send data byte
    I2CMasterDataPut(I2C1_BASE, ISL29023_COMMANDI_ONEIR | ISL29023_COMMANDI_PERSIST1);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Wait for measurement to complete
    switch(this->resSetting){
        case 65536:
            SysCtlDelay(SYSCLOCKFREQ/3/11);
            break;
        case 4096:
            SysCtlDelay(SYSCLOCKFREQ/3/166);
            break;
        case 256:
            SysCtlDelay(SYSCLOCKFREQ/3/250);
            break;
        case 16:
            SysCtlDelay(SYSCLOCKFREQ/3/250);
            break;
        default:
            SysCtlDelay(SYSCLOCKFREQ/3/11);
            break;
    }

    // Send start, configure to write, send LSB register
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, false);
    I2CMasterDataPut(I2C1_BASE, ISL29023_REG_DATALSB);
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_SEND_START);

    // Wait for bus to free
    while(I2CMasterBusy(I2C1_BASE)){}

    // Send restart, configure to read
    I2CMasterSlaveAddrSet(I2C1_BASE, ISL29023_I2C_ADDRESS, true);

    // Read LSB
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);
    while(I2CMasterBusy(I2C1_BASE)){}
    this->rawVals[1] = I2CMasterDataGet(I2C1_BASE);

    // Read MSB
    I2CMasterControl(I2C1_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
    while(I2CMasterBusy(I2C1_BASE)){}
    this->rawVals[0] = I2CMasterDataGet(I2C1_BASE);
}

void Light_Interface::ISL29023GetIR(){
    ISL29023GetRawIR();
    this->irVal = ((float)((this->rawVals[0] << 8) | this->rawVals[1])) / this->beta;
}
