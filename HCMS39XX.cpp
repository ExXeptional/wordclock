#include <HCMS39XX.h>
#include "ASCII_Char_Set.h"

HCMS39XX::HCMS39XX()
{}

void HCMS39XX::writeString(char str[])
{
    uint8_t* dotOutArr = NULL;

    clkHigh();
    waitNoTicks(4);
    selectDotRegister();
    waitNoTicks(4);
    chipEnable();
    waitNoTicks(4);
    clkLow();
    waitNoTicks(4);

    for(int k = 0;k<strlen(str);k++)
    {
        dotOutArr = encodeChar(str[k]);
        for (int i = 0; i<5;i++)
        {
            for(int j = 7; j>=0;j--)
            {
                clkLow();
                waitNoTicks(4);
                GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, (dotOutArr[i] >> j) & 0x01);
                waitNoTicks(4);
                clkHigh();
                waitNoTicks(4);
            }
        }
    }


    chipDisable();
    waitNoTicks(4);
    clkLow();
    waitNoTicks(4);
}

void HCMS39XX::write4Chars(uint8_t text[4])
{
    uint8_t* dotOutArr = NULL;

    clkHigh();
    waitNoTicks(8);
    selectDotRegister();
    waitNoTicks(8);
    chipEnable();
    waitNoTicks(8);
    clkLow();
    waitNoTicks(10);

    for(int k = 0;k<4;k++)
    {
        dotOutArr = encodeChar(text[k]);
        for (int i = 0; i<5;i++)
        {
            for(int j = 7; j>=0;j--)
            {
                clkLow();
                waitNoTicks(8);
                GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, (dotOutArr[i] >> j) & 0x01);
                waitNoTicks(8);
                clkHigh();
                waitNoTicks(10);
            }
        }
    }


    chipDisable();
    waitNoTicks(8);
    clkLow();
    waitNoTicks(8);
}

void HCMS39XX::writeChar(uint8_t c)
{
    uint8_t* dotOutArr = encodeChar(c);

    clkHigh();
    waitNoTicks(4);
    selectDotRegister();
    waitNoTicks(4);
    chipEnable();
    waitNoTicks(2);
    clkLow();
    waitNoTicks(10);

    for (int i = 0; i<5;i++)
    {
        for(int j = 7; j>=0;j--)
        {
            clkLow();
            waitNoTicks(8);
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, (dotOutArr[i] >> j) & 0x01);
            waitNoTicks(2);
            clkHigh();
            waitNoTicks(10);
        }
    }

    chipDisable();
    waitNoTicks(4);
    clkLow();
    waitNoTicks(4);
}

//Level only from 0-15
void HCMS39XX::setBrightness(uint8_t level)
{
    if(level > 15) level = 15;
    setControlWord0(PEAK_PIX_CURR, false, level);
}

void HCMS39XX::setupGPIO()
{
    /** Pinbelegung:
     *  DATA IN: PB0 Gelb
     *  CLOCK IN: PB1 Blau
     *  Register Select: PB2 Gr�n
     *  /Chip Enable: PB3 Wei�
     *  /RESET: RST Orange
     *  Blank: PE4 Lila
     **/
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
    //PortB
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0);
    GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_4);
}

void HCMS39XX::initialize()
{
    setupGPIO();

    // Clk, DataIn, RS, BL auf Lo, CE auf Hi
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4|GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_1|GPIO_PIN_0, 0x08);


    uint8_t text[] ={' ',' ',' ',' '};
    for(int i = 0;i< DISPLAY_NUMBER;i++)
    {
        //Alle Daten auf 0
        write4Chars(text);

        //Dout auf parallel mode
        setControlWord1(true, false);
    }

    //Maximale Helligkeit
    setBrightness(0xf);
}

void HCMS39XX::displayTest()
{
    uint8_t c = ' ';
    for(int i=0;i<100;i++)
    {
        writeChar((uint8_t)(c+i));
        SysCtlDelay( (SYSCLOCKFREQ/(3*1000))*200 ) ;
    }
}

void HCMS39XX::setControlWord0(uint8_t peak_curr, bool sleep, uint8_t brightness)
{
    uint8_t data = 0;
    //     select CW0    2 bits 0b11 = 100%       disables int osz  4 bits 0xf=100%
    data |= (0 << 7) | ((peak_curr & 0x3) << 4) | (sleep?0:1 << 6) | (brightness & 0xf);

    clkHigh();
    waitNoTicks(4);
    selectControlRegister();
    waitNoTicks(4);
    chipEnable();
    waitNoTicks(2);
    clkLow();
    waitNoTicks(10);

    for(int j = 7; j>=0;j--)
    {
        clkLow();
        waitNoTicks(8);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, (data >> j) & 0x01);
        waitNoTicks(2);
        clkHigh();
        waitNoTicks(10);

    }
    clkLow();
    waitNoTicks(4);
    chipDisable();
    waitNoTicks(20);
}

void HCMS39XX::setControlWord1(bool doutSync, bool extOsziPresc)
{
    uint8_t data = 0;
    //      select CW1  oszi prescaler 0=1,1=8     Dout sync with Din only for control reg
    data |= (1 << 7) | (extOsziPresc?1:0 << 1) | (doutSync?1:0);

    clkHigh();
    waitNoTicks(4);
    selectControlRegister();
    waitNoTicks(4);
    chipEnable();
    waitNoTicks(2);
    clkLow();
    waitNoTicks(10);

    for(int j = 7; j>=0;j--)
    {
        clkLow();
        waitNoTicks(8);
        GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, (data >> j) & 0x01);
        waitNoTicks(2);
        clkHigh();
        waitNoTicks(10);

    }
    clkLow();
    waitNoTicks(1);
    chipDisable();
    waitNoTicks(1);
}

void HCMS39XX::blankDisplay()
{
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0x10);
}

void HCMS39XX::unBlankDisplay()
{
    GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_4, 0);
}

void HCMS39XX::selectDotRegister()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0);
    waitNoTicks(2);
}

void HCMS39XX::selectControlRegister()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_2, 0x04);
    waitNoTicks(2);
}

void HCMS39XX::chipEnable()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);
}

void HCMS39XX::chipDisable()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0x08);
}

void HCMS39XX::clkLow()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, 0);
}

void HCMS39XX::clkHigh()
{
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_1, 0x02);
}

//Returns the Bitvector corresponding to the char
uint8_t* HCMS39XX::encodeChar(char c)
{
    uint8_t* dotArr;
   switch( c )
   {
        case ' ': dotArr = _ASCII_BNK; break;
        case '!': dotArr = _ASCII_EXM; break;
        case '\"': dotArr = _ASCII_ANF; break;
        case '#': dotArr = _ASCII_HSH; break;
        case '$': dotArr = _ASCII_DOL; break;
        case '%': dotArr = _ASCII_PRZ; break;
        case '&': dotArr = _ASCII_AMP; break;
        case '\'': dotArr = _ASCII_HKM; break;
        case '(': dotArr = _ASCII_LRK; break;
        case ')': dotArr = _ASCII_RRK; break;
        case '*': dotArr = _ASCII_AST; break;
        case '+': dotArr = _ASCII_PLS; break;
        case ',': dotArr = _ASCII_LKM; break;
        case '-': dotArr = _ASCII_BDS; break;
        case '.': dotArr = _ASCII_DOT; break;
        case '/': dotArr = _ASCII_SL; break;
        case '0': dotArr = _ASCII_0; break;
        case '1': dotArr = _ASCII_1; break;
        case '2': dotArr = _ASCII_2; break;
        case '3': dotArr = _ASCII_3; break;
        case '4': dotArr = _ASCII_4; break;
        case '5': dotArr = _ASCII_5; break;
        case '6': dotArr = _ASCII_6; break;
        case '7': dotArr = _ASCII_7; break;
        case '8': dotArr = _ASCII_8; break;
        case '9': dotArr = _ASCII_9; break;
        case ':': dotArr = _ASCII_DPT; break;
        case '<': dotArr = _ASCII_LSK; break;
        case '=': dotArr = _ASCII_EQ; break;
        case '>': dotArr = _ASCII_RSK; break;
        case '?': dotArr = _ASCII_QSM; break;
        case '@': dotArr = _ASCII_AT; break;
        case 'A': dotArr = _ASCII_A; break;
        case 'B': dotArr = _ASCII_B; break;
        case 'C': dotArr = _ASCII_C; break;
        case 'D': dotArr = _ASCII_D; break;
        case 'E': dotArr = _ASCII_E; break;
        case 'F': dotArr = _ASCII_F; break;
        case 'G': dotArr = _ASCII_G; break;
        case 'H': dotArr = _ASCII_H; break;
        case 'I': dotArr = _ASCII_I; break;
        case 'J': dotArr = _ASCII_J; break;
        case 'K': dotArr = _ASCII_K; break;
        case 'L': dotArr = _ASCII_L; break;
        case 'M': dotArr = _ASCII_M; break;
        case 'N': dotArr = _ASCII_N; break;
        case 'O': dotArr = _ASCII_O; break;
        case 'P': dotArr = _ASCII_P; break;
        case 'Q': dotArr = _ASCII_Q; break;
        case 'R': dotArr = _ASCII_R; break;
        case 'S': dotArr = _ASCII_S; break;
        case 'T': dotArr = _ASCII_T; break;
        case 'U': dotArr = _ASCII_U; break;
        case 'V': dotArr = _ASCII_V; break;
        case 'W': dotArr = _ASCII_W; break;
        case 'X': dotArr = _ASCII_X; break;
        case 'Y': dotArr = _ASCII_Y; break;
        case 'Z': dotArr = _ASCII_Z; break;
        case '[': dotArr = _ASCII_LSK; break;
        case '\\': dotArr = _ASCII_BSL; break;
        case ']': dotArr = _ASCII_RSK; break;
        case '^': dotArr = _ASCII_PFO; break;
        case '_': dotArr = _ASCII__; break;
        case '`': dotArr = _ASCII_TIC; break;
        case 'a': dotArr = _ASCII_a; break;
        case 'b': dotArr = _ASCII_b; break;
        case 'c': dotArr = _ASCII_c; break;
        case 'd': dotArr = _ASCII_d; break;
        case 'e': dotArr = _ASCII_e; break;
        case 'f': dotArr = _ASCII_f; break;
        case 'g': dotArr = _ASCII_g; break;
        case 'h': dotArr = _ASCII_h; break;
        case 'i': dotArr = _ASCII_i; break;
        case 'j': dotArr = _ASCII_j; break;
        case 'k': dotArr = _ASCII_k; break;
        case 'l': dotArr = _ASCII_l; break;
        case 'm': dotArr = _ASCII_m; break;
        case 'n': dotArr = _ASCII_n; break;
        case 'o': dotArr = _ASCII_o; break;
        case 'p': dotArr = _ASCII_p; break;
        case 'q': dotArr = _ASCII_q; break;
        case 'r': dotArr = _ASCII_r; break;
        case 's': dotArr = _ASCII_s; break;
        case 't': dotArr = _ASCII_t; break;
        case 'u': dotArr = _ASCII_u; break;
        case 'v': dotArr = _ASCII_v; break;
        case 'w': dotArr = _ASCII_w; break;
        case 'x': dotArr = _ASCII_x; break;
        case 'y': dotArr = _ASCII_y; break;
        case 'z': dotArr = _ASCII_z; break;
        case '{': dotArr = _ASCII_LGK; break;
        case '|': dotArr = _ASCII_PIP; break;
        case '}': dotArr = _ASCII_RGK; break;
        case '~': dotArr = _ASCII_TIL; break;
        case 0xDB: dotArr = _ASCII_BLK; break;
        case 'ä': dotArr = _ASCII_auml; break;
        case 'ö': dotArr = _ASCII_ouml; break;
        case 'ü': dotArr = _ASCII_uuml; break;
        case 'Ä': dotArr = _ASCII_AUML; break;
        case 'Ö': dotArr = _ASCII_OUML; break;
        case 'Ü': dotArr = _ASCII_UUML; break;
        default:  dotArr = _ASCII_BNK; break;
   }
 return dotArr;
}

void HCMS39XX::waitNoTicks(int no)
{
    // Dauer = 3 Cycles = 62,5 ns
    SysCtlDelay((int)ceil(no/3.0));
}
