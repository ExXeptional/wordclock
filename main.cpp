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
#include "driverlib/timer.h"
#include "driverlib/adc.h"

#include "ISL29023_Interface.h"

#include "HCMS39XX.h"
#include "DS3234.h"

/*
 * TODO:
 * button interrupt und blinken noch fixen
 * Licht Ic persistence aktivieren (8)
 * LichtIC defekt????
 * String builder alle const in ein const array schreiben und von dort laden
 */

//Defines
#define BLINK_SPEED 4 //Toggles per Second
#define SYSCLOCK 80000000
#define ATOMIC_REQUEST_DELAY 0060 //Wiederholrate für NMEA

//Datatypes
enum DisplayModes {NORM_MODE=0, DAY_BLANK, DOW_BLANK, MON_BLANK, YEAR_BLANK, HR_BLANK, MIN_BLANK, SEC_BLANK};
enum States {INIT=0, NORM_OPERATION, SET_START, SET_HR, SET_MIN, SET_SEC, SET_YEAR, SET_MON, SET_DAY, SET_DOW, SET_FINISHED};

//Prototpes
void delay(int us);
void tmrISR();
void onSecondElapsed();
void onButtonDown();
void shiftMenu();
void incrementDateTime();
void setupPeripherals();
void Timer1BReset();
void requestAtomicTime();
void UART4IntHandler();

void stringBuilder(char* str, uint8_t day, uint8_t mon, uint16_t year, uint8_t hr, uint8_t min, uint8_t sec, uint8_t weekday, enum DisplayModes mode);

//Global Variables
enum DisplayModes dispMode;
enum States state;
static volatile bool secElapsed;
volatile bool lightMeasure;
volatile bool changed;
volatile bool debounced;
volatile bool btn1Pressed;
volatile bool btn2Pressed;
volatile int8_t brightness;
volatile uint8_t uartBuffer[255];
volatile bool gotNMEAMessage;
volatile bool appendSyncSymbol;
volatile bool dataReceived;
float lux;
int i, uartIndex;
static char text[(DISPLAY_NUMBER * 4) + 1];
static const char* timeRequestString = "$PMOTG,GGA,0060\r\n" ;//Mit wiederholrate alle 60 sekunden
//static const char textBits = {' ','.',',','E','s','i','t',}
Time t, temp;
DS3234 clk(0);
HCMS39XX disp;
Light_Interface lightInt;

//Main Program
int main()
    {
    state = INIT;
    dispMode = NORM_MODE;
	while(1)
	{
	    switch(state)
	    {
	    case INIT:
	        changed = true;
	        secElapsed = false;
	        debounced = true;
	        btn1Pressed = false;
	        btn2Pressed = false;
	        lightMeasure = false;
	        dataReceived = false;
	        gotNMEAMessage = false;
	        appendSyncSymbol = false;
	        brightness = 0xf;
	        lux = 32000.0;
	        disp.initialize();
	        disp.setBrightness(0xf);
	        //disp.displayTest();
	        lightInt.initialize();
	        clk.begin();
	        setupPeripherals();
	        //t = clk.getTime();
	        //Anhand des MSB im Statusregister des DS3234 kann man erkennen, ob die Zeit noch aktuell ist, evtl abfrage um dann direkt in den Setmodus zu gehen
	        //          "                                                        "
            strcpy(text,"++++++++Wörteruhr v2    Raphael Kriegl  VMC WS17/18     ");
            disp.writeString(text);
            //requestAtomicTime();
            delay(2000000);
            TimerEnable(WTIMER0_BASE, TIMER_B);
            state = NORM_OPERATION;
            break;
        case NORM_OPERATION:
            if(secElapsed)
            {
                t = clk.getTime();
                changed = true;
                secElapsed = false;
                if(gotNMEAMessage) {
                    appendSyncSymbol = true;
                    gotNMEAMessage = false;
                } else {
                    appendSyncSymbol = false;
                }
               // GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_5);
            }
            if(lightMeasure)
            {
                lux = lightInt.getLux();
//                if(lux > 100) brightness = 15;
//                else brightness = ceil(lux/6.667);
                brightness = ceil(log2(lux));
                if (brightness > 15) brightness = 15;
                disp.setBrightness(brightness);
                lightMeasure = false;
            }
            break;
        case SET_START:
            TimerDisable(WTIMER0_BASE, TIMER_B);
            TimerEnable(WTIMER1_BASE, TIMER_B);
            GPIOIntDisable(GPIO_PORTA_BASE, GPIO_PIN_5);
            disp.setBrightness(0xf);
            state = SET_HR;
            break;
        case SET_HR: case SET_DAY: case SET_YEAR: case SET_SEC: case SET_MIN: case SET_MON:
            break;
        case SET_FINISHED:
            TimerDisable(WTIMER1_BASE, TIMER_B);
            clk.setDate(t.date, t.mon, t.year);
            clk.setTime(t.hour, t.min, t.sec);
            i = t.year;
            // Berechnung mit formel aus wikipedia
            if (t.mon < 3) i = i - 1;
            t.dow = ((t.date + (int)floor(2.6 * ((t.mon + 9) % 12 + 1) - 0.2) + i % 100 + (int)floor(i % 100 / 4) + (int)floor(i / 400) - 2 * (int)floor(i / 100) - 1) % 7 + 7) % 7 + 1;
            clk.setDOW(t.dow);
            state = NORM_OPERATION;
            dispMode = NORM_MODE;
            changed = true;
            TimerEnable(WTIMER0_BASE, TIMER_B);
            GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_5);
            break;
        default:
            break;
        }

        if (gotNMEAMessage) {
            //!ZARMC135548A0000.0000N00000.0000E00.000.02312170.0EA
            //$GPRMC,hhmmss.ss,a,ddmm.mmmm,n,dddmm.mmmm,w,z.z,y.y,ddmmyy,d.d,v*CC<CR><LF>
            temp.hour = (uartBuffer[7] * 10) + uartBuffer[8];
            temp.min = (uartBuffer[9] * 10) + uartBuffer[10];
            temp.sec = (uartBuffer[11] * 10) + uartBuffer[12];
            temp.date = (uartBuffer[42] * 10) + uartBuffer[43];
            temp.mon = (uartBuffer[44] * 10) + uartBuffer[45];
            temp.year = 2000 + (uartBuffer[46] * 10) + uartBuffer[47];
            state = SET_FINISHED;
        }

        if (changed)
        {
            IntDisable(INT_WTIMER1B);
            IntDisable(INT_GPIOA);
            stringBuilder(text, t.date, t.mon, t.year, t.hour, t.min, t.sec, t.dow, dispMode);
            disp.writeString(text);
            changed = false;
            IntEnable(INT_WTIMER1B);
            IntEnable(INT_GPIOA);
        }


    }
    return 0;
}

/**
* Display-Edit-Funktionen
*/

/*
* Erstellt den String der auf die Displays geschrieben wird mittels der �bergebenen Parameter
* Wird speziell auf den Anwendungsfall zugeschnitten; "mode" kann einzelne Daten ausblenden, dass es aussieht als ob sie blinken
*/
void stringBuilder(char* str, uint8_t day, uint8_t mon, uint16_t year, uint8_t hr, uint8_t min, uint8_t sec, uint8_t weekday, enum DisplayModes mode)
{
   str[0] = 'E'; str[1]= 's'; str[2] = ' '; str[3] = 'i'; str[4] = 's'; str[5] = 't'; str[6] = ' '; str[7] = ' ';

   str[8] = (dispMode!=HR_BLANK && hr>9)?(hr/10) % 10 +'0':' ';
   str[9] = (dispMode!=HR_BLANK)?hr % 10 + '0':' ';
   str[10] = '.';
   str[11] = (dispMode!=MIN_BLANK)?(min / 10) % 10 + '0':' ';
   str[12] = (dispMode!=MIN_BLANK)?min % 10 + '0':' ';
   str[13] = '.';
   str[14] = (dispMode!=SEC_BLANK)?(sec / 10) % 10 + '0':' ';
   str[15] = (dispMode!=SEC_BLANK)?sec % 10 + '0':' ';
   str[16] = ' ';
   str[17] = 'U';
   str[18] = 'h';
   str[19] = 'r';
   str[20] = ',';
   str[21] = ' ';
   str[22] = 'a';
   str[23] = 'm';

   switch(weekday)
   {
       case 1: str[24] = 'M'; str[25] = 'o'; str[26] = 'n'; str[27] = 't'; str[28] = 'a'; str[29] = 'g'; str[30] = ','; str[31] = ' '; str[32] = 'd'; str[33] = 'e'; str[34] = 'n'; str[35] = ' '; str[36] = ' '; str[37] = ' '; str[38] = ' '; str[39] = ' '; break;
       case 2: str[24] = 'D'; str[25] = 'i'; str[26] = 'e'; str[27] = 'n'; str[28] = 's'; str[29] = 't'; str[30] = 'a'; str[31] = 'g'; str[32] = ','; str[33] = ' '; str[34] = 'd'; str[35] = 'e'; str[36] = 'n'; str[37] = ' '; str[38] = ' '; str[39] = ' '; break;
       case 3: str[24] = 'M'; str[25] = 'i'; str[26] = 't'; str[27] = 't'; str[28] = 'w'; str[29] = 'o'; str[30] = 'c'; str[31] = 'h'; str[32] = ','; str[33] = ' '; str[34] = 'd'; str[35] = 'e'; str[36] = 'n'; str[37] = ' '; str[38] = ' '; str[39] = ' '; break;
       case 4: str[24] = 'D'; str[25] = 'o'; str[26] = 'n'; str[27] = 'n'; str[28] = 'e'; str[29] = 'r'; str[30] = 's'; str[31] = 't'; str[32] = 'a'; str[33] = 'g'; str[34] = ','; str[35] = ' '; str[36] = 'd'; str[37] = 'e'; str[38] = 'n'; str[39] = ' '; break;
       case 5: str[24] = 'F'; str[25] = 'r'; str[26] = 'e'; str[27] = 'i'; str[28] = 't'; str[29] = 'a'; str[30] = 'g'; str[31] = ','; str[32] = ' '; str[33] = 'd'; str[34] = 'e'; str[35] = 'n'; str[36] = ' '; str[37] = ' '; str[38] = ' '; str[39] = ' '; break;
       case 6: str[24] = 'S'; str[25] = 'a'; str[26] = 'm'; str[27] = 's'; str[28] = 't'; str[29] = 'a'; str[30] = 'g'; str[31] = ','; str[32] = ' '; str[33] = 'd'; str[34] = 'e'; str[35] = 'n'; str[36] = ' '; str[37] = ' '; str[38] = ' '; str[39] = ' '; break;
       case 7: str[24] = 'S'; str[25] = 'o'; str[26] = 'n'; str[27] = 'n'; str[28] = 't'; str[29] = 'a'; str[30] = 'g'; str[31] = ','; str[32] = ' '; str[33] = 'd'; str[34] = 'e'; str[35] = 'n'; str[36] = ' '; str[37] = ' '; str[38] = ' '; str[39] = ' '; break;
       default:str[24] = ' '; str[25] = ' '; str[26] = ' '; str[27] = ' '; str[28] = ' '; str[29] = ' '; str[30] = ' '; str[31] = ' '; str[32] = ' '; str[33] = ' '; str[34] = ','; str[35] = ' '; str[36] = 'd'; str[37] = 'e'; str[38] = 'n'; str[39] = ' '; break;
   }

   str[40] = (dispMode!=DAY_BLANK)?(day/10) % 10 +'0':' ';
   str[41] = (dispMode!=DAY_BLANK)?day % 10 + '0':' ';
   str[42] = '.';
   str[43] = (dispMode!=MON_BLANK)?(mon / 10) % 10 + '0':' ';
   str[44] = (dispMode!=MON_BLANK)?mon % 10 + '0':' ';
   str[45] = '.';
   str[46] = (dispMode!=YEAR_BLANK)?(year / 1000) % 10 + '0':' ';
   str[47] = (dispMode!=YEAR_BLANK)?(year / 100) % 10 + '0':' ';
   str[48] = (dispMode!=YEAR_BLANK)?(year / 10) % 10 + '0':' ';
   str[49] = (dispMode!=YEAR_BLANK)?year % 10 + '0':' ';
   str[50] = ' ';
   str[51] = ' ';
   str[52] = ' ';
   str[53] = ' ';
   str[54] = ' ';
   str[55] = ' ';
   str[56] = 0;
}

/*
* Sch�lt die Einstellmodi durch
*/
void shiftMenu()
{
    switch(state)
    {
    case NORM_OPERATION:
        state = SET_START;
        break;
    case SET_HR:
        state = SET_MIN;
        break;
    case SET_MIN:
        state = SET_SEC;
        break;
    case SET_SEC:
        state = SET_YEAR;
        break;
    case SET_YEAR:
        state = SET_MON;
        break;
    case SET_MON:
        state = SET_DAY;
        break;
    case SET_DAY:
        state = SET_FINISHED;
        break;
    default:
        //state = NORM_OPERATION;
        break;
    }
//    Timer1BReset();
//    dispMode=NORM_MODE;
//    changed = true;
}

/*
* Erh�ht den einzustellenden Wert in den entsprechenden Grenzen
*/
void incrementDateTime()
{
   switch(state)
   {
   case SET_DAY:
       if(t.mon==1 || t.mon==3 || t.mon==5 || t.mon==7 || t.mon==8 || t.mon==10 || t.mon==12)
       {
           if(t.date >= 31) t.date = 1;
           else t.date ++;
       }
       else if(t.mon == 2)
       {
           //Schaltjahre
           if((t.year % 4 == 0 && t.year % 100 != 0) || t.year % 400 ==0)
           {
               if(t.date >=29) t.date = 1;
               else t.date ++;
           }
           else
           {
               if(t.date >=28) t.date = 1;
               else t.date ++;
           }
       }
       else
       {
           if(t.date >= 30) t.date = 1;
           else t.date ++;
       }

        break;
   case SET_DOW: t.dow = (t.dow>=7)?1:(t.dow+1); break;
   case SET_MON: t.mon = (t.mon>=12)?1:(t.mon+1); break;
   case SET_YEAR: t.year = (t.year>=2099)?2000:(t.year+1); break;
   case SET_HR: t.hour = (t.hour>=23)?0:(t.hour+1); break;
   case SET_MIN: t.min = (t.min>=59)?0:(t.min+1); break;
   case SET_SEC: t.sec = (t.sec>=59)?0:(t.sec+1); break;
   default: dispMode = NORM_MODE; break;
   }
//   Timer1BReset();
//   dispMode = NORM_MODE;
//   changed = true;
}


/**
* Interrupt-Handler Funktionen
*/

/*
* Handler f�r Button-Interrupt
* Startet den Entprell-Timer
*/
void onButtonDown()
{
    IntMasterDisable();
    if(debounced)
    {
        //Menu durchschalten
        if (GPIOIntStatus(GPIO_PORTE_BASE, false) & GPIO_PIN_2)
        {
            btn1Pressed = true;
        }
        if (GPIOIntStatus(GPIO_PORTE_BASE, false) & GPIO_PIN_3)
        {
            btn2Pressed = true;
        }
        debounced = false;
        //In der Int-Routine wird entschieden, ob der Button tats�chlich gedr�ckt wurde
        TimerEnable(WTIMER0_BASE, TIMER_A);
    }
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2);
    IntMasterEnable();
}

/*
* Interrupt-Handler f�r den Entprell-Timer
* Evaluiert den momentanen Spannungswert an den Button-Pins, falls Low ist der "Dr�cker" best�tigt und die entspr Funktion wird ausgef�hrt
*/
void WTimer0AIntHandler()
{
    IntMasterDisable();
    TimerIntClear(WTIMER0_BASE, TIMER_TIMA_TIMEOUT);
    if(btn1Pressed && (GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2)&GPIO_PIN_2)!=GPIO_PIN_2)
    {
        shiftMenu();
    }
    if(btn2Pressed && (GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_3)&GPIO_PIN_3)!=GPIO_PIN_3)
    {
        incrementDateTime();
    }
    btn1Pressed = false;
    btn2Pressed = false;
    debounced = true;
    IntMasterEnable();
}

/*
* Wird vom Sek�ndlichen Pin Interrupt des RTC Bausteins aufgerufen
* Setzt eine Flag, die signalisiert, dass die Zeit neu abgefragt werden muss
*/
void onSecondElapsed()
{
    secElapsed = true;
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_5);
}

/*
* Interrupt-Handler f�r den "Blink-Timer"
* Wechselt zwischen Normalem Display und der Ausblendung einer Bestimmten Zahl je nach State
*/
void WTimer1BIntHandler(void)
{
    IntMasterDisable();
    //
    // Clear the timer interrupt flag.
    //
    TimerIntClear(WTIMER1_BASE, TIMER_TIMB_TIMEOUT);

    if(dispMode == NORM_MODE)
    {
        switch(state)
        {
        case SET_DAY:dispMode = DAY_BLANK; break;
        case SET_MON:dispMode = MON_BLANK; break;
        case SET_YEAR:dispMode = YEAR_BLANK; break;
        case SET_HR:dispMode = HR_BLANK; break;
        case SET_MIN:dispMode = MIN_BLANK; break;
        case SET_SEC:dispMode = SEC_BLANK; break;
        default: dispMode = NORM_MODE; break;
        }
    }
    else
        dispMode = NORM_MODE;

    changed = true;
    IntMasterEnable();
}

/*
* Interrupt-Handler f�r den "Helligkeistmessungstimer"
* Wechselt zwischen Normalem Display und der Ausblendung einer Bestimmten Zahl je nach State
*/
void WTimer0BIntHandler(void)
{
    IntMasterDisable();
    //
    // Clear the timer interrupt flag.
    //
    TimerIntClear(WTIMER0_BASE, TIMER_TIMB_TIMEOUT);

    //Setze Flag
    lightMeasure = true;

    IntMasterEnable();
}


/*
 * Stellt Zeitnafrage an Rubidium Uhr via NMEA über UART4
 */
void requestAtomicTime() {
    int j = 0;
    while(timeRequestString[j] != 0) {
        UARTCharPut(UART4_BASE, timeRequestString[j]);
        j++;
    }
}

/*
 * Interrupt-Handler für die UART Schnittstelle (NMEA)
 * zur Rubidium Uhr
 */
void UART4IntHandler(void)
{
    IntMasterDisable();

    UARTIntClear(UART4_BASE, UART_INT_RX);

    while(UARTCharsAvail(UART4_BASE))
    {
        uartBuffer[uartIndex] = UARTCharGetNonBlocking(UART4_BASE);

        if(uartBuffer[uartIndex] == 0xA) { // Got line feed, end of message
            uartIndex = 0;
            gotNMEAMessage = true;
            continue;
        }
        uartIndex++;
    }
    IntMasterEnable();
}


/**
* Sonstige Funktionen
*/

/*
* Stellt alle Controller Funktionen ein, die ben�tigt werden
*/
void setupPeripherals()
{
    //Sysclock
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |SYSCTL_XTAL_16MHZ);

    //Timer f�r Blinken
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER1);
    TimerConfigure(WTIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_B_PERIODIC);
    //TimerPrescaleSet(WTIMER1_BASE, TIMER_B, 0xff);
    TimerLoadSet(WTIMER1_BASE, TIMER_B, SYSCLOCK/BLINK_SPEED);
    TimerIntRegister(WTIMER1_BASE, TIMER_B, WTimer1BIntHandler);
    TimerIntEnable(WTIMER1_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_WTIMER1B);

    //Timer f�r Entprellen
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);
    TimerConfigure(WTIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_ONE_SHOT | TIMER_CFG_B_PERIODIC);
    TimerLoadSet(WTIMER0_BASE, TIMER_A, SYSCLOCK/1000 * 20);//20 ms
    TimerIntRegister(WTIMER0_BASE, TIMER_A, WTimer0AIntHandler);
    TimerIntEnable(WTIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_WTIMER0A);

    //Timer f�r Helligkeitsmessung
    TimerLoadSet(WTIMER0_BASE, TIMER_B, SYSCLOCK/5); // 1/5 Sekunde
    TimerIntRegister(WTIMER0_BASE, TIMER_B, WTimer0BIntHandler);
    TimerIntEnable(WTIMER0_BASE, TIMER_TIMB_TIMEOUT);
    IntEnable(INT_WTIMER0B);

    //Button Interrupts, 2 f�r Setup, 3 um Werte zu erh�hen
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2);
    GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); //WPU == Weak PullUp
    GPIOIntDisable(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2);
    GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2);
    GPIOIntRegister(GPIO_PORTE_BASE, onButtonDown);
    GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_2, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTE_BASE, GPIO_INT_PIN_3 | GPIO_INT_PIN_2);
    IntEnable(INT_GPIOE);

    //Sek�ndlicher Clock Pin Interrupt
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_5);
    GPIOPadConfigSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // Mit pullup, da der RTC-Baustein nur Open Drain arbeitet
    GPIOIntDisable(GPIO_PORTA_BASE, GPIO_PIN_5);
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_5);
    GPIOIntRegister(GPIO_PORTA_BASE, onSecondElapsed);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTA_BASE, GPIO_INT_PIN_5);
    IntEnable(INT_GPIOA);

    //UART4 Schnittstelle
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART4));
    GPIOPinConfigure(GPIO_PC4_U4RX);
    GPIOPinConfigure(GPIO_PC5_U4TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);


    UARTFIFOEnable(UART4_BASE);
    UARTFIFOLevelSet(UART4_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    UARTConfigSetExpClk(UART4_BASE, UART_CLOCK_SYSTEM, 4800, UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE | UART_CONFIG_WLEN_8);
    UARTEnable(UART4_BASE);
    UARTIntRegister(UART4_BASE, UART4IntHandler);
    IntEnable(INT_UART4);
    UARTIntEnable(UART4_BASE, UART_INT_RX);

    IntMasterEnable();
}

/*
 * Timer1B Reset stellt den maximalen Wert wieder her und startet den Tiemr neu
 */
void Timer1BReset()
{
    TimerDisable(WTIMER1_BASE, TIMER_B);
    TimerLoadSet(WTIMER1_BASE, TIMER_B, SYSCLOCK/BLINK_SPEED);
    TimerEnable(WTIMER1_BASE, TIMER_B);
}

/*
* Delay Funktion
* Verz�gerung in us
*/
void delay(int us)
{
    // Not 100% accurate
    SysCtlDelay( (SYSCLOCK/(3*1000000))*us ) ;
}
