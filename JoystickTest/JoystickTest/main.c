
/*
#include <avr/io.h>

enum States { Init, Read, Display } State;
unsigned char outputC = 0x00;
//unsigned char outputD = 0x00;
unsigned short readADC = 0x0000;

void Tick() {
	switch (State) {
		case Init:
			outputC = 0x00;
			//outputD = 0x00;
			State = Read;
			break;
		case Read:
			readADC = ADC;
			//readADC = 0xF0A;
			outputC = (char)readADC;
			//outputD = (char)((readADC >> 8) & 0x03);
			State = Display;
			break;
		case Display:
			PORTC = outputC;
			//PORTD = outputD;
			State = Read;
			break;
	}
}
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

int main(void)
{
	DDRA = 0x00;
	PORTA = 0xFF;
	
	DDRC = 0xFF;
	PORTC = 0x00;
	//DDRD = 0xFF;
	//PORTD = 0x00;
	
	State = Init;
	
	ADC_init();
    while(1)
    {
        Tick(); 
    }
}
*/

/*
#include <avr/io.h>
#include "timer.h"
#include "scheduler.h"
#include "io.c"

typedef unsigned char u_char;
typedef unsigned long int u_lint;

//cursor position
u_char cursor[] = {2, 4};
//flag for lighting  the cursor
u_char cursor_on;
//flag for blinking the cursor
u_char cursor_blink;
//values that store the input from the joystick
int16_t U_D;
int16_t L_R;

int TickFct_CursorPos_H(int state);
int TickFct_CursorPos_V(int state);

//Initialize the ADC
void ADC_init()
{
	ADMUX = (1<<REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

//Read from the ADC channel ch
uint16_t ReadADC(uint8_t ch)
{
	//Select ADC Channel
	ch &= 0x07;
	ADMUX = (ADMUX & 0xF8) | ch;

	//Start Single conversion
	ADCSRA |= (1 << ADSC);

	//Wait for conversion to complete
	while(ADCSRA & (1 << ADSC));

	//Clear ADIF
	ADCSRA |= (1<<ADIF);
	return (ADC);
}

int main(void)
{
	DDRA = 0x83; PORTA = 0x7C;
	
	//task periods
	u_lint CursorPos_H_calc = 200;
	u_lint CursorPos_V_calc = 200;
	
	//calculate GDC
	u_lint tmpGCD = 1;
	tmpGCD = findGCD(tmpGCD, CursorPos_H_calc);
	tmpGCD = findGCD(tmpGCD, CursorPos_V_calc);
	
	u_lint GCD = tmpGCD;
	
	//recalculate periods
	u_lint CursorPos_H_period = CursorPos_H_calc/GCD;
	u_lint CursorPos_V_period = CursorPos_V_calc/GCD;
	
	//task array declaration
	static task taskCursor_Pos_H,
	taskCursor_Pos_V;
	task *tasks[] = { &taskCursor_Pos_H, &taskCursor_Pos_V };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	//Task Cursor_Pos_H
	taskCursor_Pos_H.state = -1;
	taskCursor_Pos_H.period = CursorPos_H_period;
	taskCursor_Pos_H.elapsedTime = CursorPos_H_period;
	taskCursor_Pos_H.TickFct = &TickFct_CursorPos_H;
	
	//Task Cursor_Pos_V
	taskCursor_Pos_V.state = -1;
	taskCursor_Pos_V.period = CursorPos_V_period;
	taskCursor_Pos_V.elapsedTime = CursorPos_V_period;
	taskCursor_Pos_V.TickFct = &TickFct_CursorPos_V;
	
	//Set Timer
	TimerSet(GCD);
	TimerOn();
	
	ADC_init();
	
	unsigned short i;
	while(1)
	{
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime == tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
				// Reset the elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
}

//handle up-down movement
void moveU_D(int16_t U_D)
{
	u_char tmp = cursor[1];
	if(U_D > 0)
	{
		tmp = (tmp == 7) ? 0 : tmp+1;
	}
	else if(U_D < 0)
	{
		tmp = (tmp == 0) ? 7 : tmp-1;
	}
	cursor[1] = tmp;
}

//handle left-right movement
void moveL_R(int16_t L_R)
{
	u_char tmp = cursor[0];
	if(L_R < 0)
	{
		tmp = (tmp == 4) ? 0 : tmp+1;
	}
	else if(L_R > 0)
	{
		tmp = (tmp == 0) ? 4 : tmp-1;
	}
	cursor[0] = tmp;
}

//determines horizontal cursor movement from joystick input
enum CPH_States {CPH_Wait, CPH_Read, CPH_Stay, CPH_Move, CPH_Ack};
int TickFct_CursorPos_H(int state)
{
	L_R = ReadADC(3);
	L_R -= 512;
	switch(state)//Transitions
	{
		case CPH_Stay:
		if((L_R <= 250) && (L_R > -249))
		{
			state = CPH_Stay;
		}
		else
		{
			state = CPH_Move;
		}
		break;
		case CPH_Move:
		if((L_R <= 250) && (L_R > -249))
		{
			state = CPH_Stay;
		}
		else
		{
			state = CPH_Move;
		}
		break;
		default:
		state = CPH_Stay;
		break;
	}
	switch(state)//Actions
	{
		case CPH_Stay:
		cursor_blink = 0;
		break;
		case CPH_Move:
		moveL_R(L_R);
		cursor_blink = 1;
		cursor_on = 1;
		break;
	}
	return state;
}

//determines vertical cursor movement from joystick input
enum CPV_States {CPV_Stay, CPV_Move};
int TickFct_CursorPos_V(int state)
{
	U_D = ReadADC(2);
	U_D -= 512;
	switch(state)//Transitions
	{
		case CPV_Stay:
		if((U_D <= 250) && (U_D > -249))
		{
			state = CPV_Stay;
		}
		else
		{
			state = CPV_Move;
		}
		break;
		case CPV_Move:
		if((U_D <= 250) && (U_D > -249))
		{
			state = CPV_Stay;
		}
		else
		{
			state = CPV_Move;
		}
		break;
		default:
		state = CPV_Stay;
		break;
	}
	switch(state)//Actions
	{
		case CPV_Stay:
		cursor_blink = 0;
		break;
		case CPV_Move:
		moveU_D(U_D);
		cursor_blink = 1;
		cursor_on = 1;
		break;
	}
	return state;
}
*/

#define F_CPU 16000000L // run CPU at 16 MHz
#define ClearBit(x,y) x &= ~_BV(y) // equivalent to cbi(x,y)
#define SetBit(x,y) x |= _BV(y) // equivalent to sbi(x,y)
// ---------------------------------------------------------------------------
// INCLUDES
#include <avr/io.h> // deal with port registers
#include <avr/interrupt.h> // deal with interrupt calls
#include <avr/pgmspace.h> // put character data into progmem
#include <util/delay.h> // used for _delay_ms function
#include <string.h> // string manipulation routines
#include <avr/sleep.h> // used for sleep functions
#include <stdlib.h>
#include "timer.h"
#include "scheduler.h"
#include "io.c"

// ---------------------------------------------------------------------------
// TYPEDEFS
typedef uint8_t byte; // I just like byte & sbyte better
typedef int8_t sbyte;

typedef unsigned char u_char;
typedef unsigned long int u_lint;

//u_char cursor[] = {2, 4};
	u_char x = 70;
	u_char y = 70;
//flag for lighting  the cursor
u_char cursor_on;
//flag for blinking the cursor
u_char cursor_blink;
//values that store the input from the joystick
int16_t U_D;
int16_t L_R;

//volatile unsigned char TimerFlag=0;

//void TimerISR() {
//	TimerFlag = 1;
//}

// ---------------------------------------------------------------------------
// GLOBAL VARIABLES
const byte FONT_CHARS[96][5] PROGMEM =
{
 { 0x00, 0x00, 0x00, 0x00, 0x00 }, // (space)
 { 0x00, 0x00, 0x5F, 0x00, 0x00 }, // !
 { 0x00, 0x07, 0x00, 0x07, 0x00 }, // "
 { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, // #
 { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, // $
 { 0x23, 0x13, 0x08, 0x64, 0x62 }, // %
 { 0x36, 0x49, 0x55, 0x22, 0x50 }, // &
 { 0x00, 0x05, 0x03, 0x00, 0x00 }, // '
 { 0x00, 0x1C, 0x22, 0x41, 0x00 }, // (
 { 0x00, 0x41, 0x22, 0x1C, 0x00 }, // )
 { 0x08, 0x2A, 0x1C, 0x2A, 0x08 }, // *
 { 0x08, 0x08, 0x3E, 0x08, 0x08 }, // +
 { 0x00, 0x50, 0x30, 0x00, 0x00 }, // ,
 { 0x08, 0x08, 0x08, 0x08, 0x08 }, // -
 { 0x00, 0x60, 0x60, 0x00, 0x00 }, // .
 { 0x20, 0x10, 0x08, 0x04, 0x02 }, // /
 { 0x3E, 0x51, 0x49, 0x45, 0x3E }, // 0
 { 0x00, 0x42, 0x7F, 0x40, 0x00 }, // 1
 { 0x42, 0x61, 0x51, 0x49, 0x46 }, // 2
 { 0x21, 0x41, 0x45, 0x4B, 0x31 }, // 3
 { 0x18, 0x14, 0x12, 0x7F, 0x10 }, // 4
 { 0x27, 0x45, 0x45, 0x45, 0x39 }, // 5
 { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, // 6
 { 0x01, 0x71, 0x09, 0x05, 0x03 }, // 7
 { 0x36, 0x49, 0x49, 0x49, 0x36 }, // 8
 { 0x06, 0x49, 0x49, 0x29, 0x1E }, // 9
 { 0x00, 0x36, 0x36, 0x00, 0x00 }, // :
 { 0x00, 0x56, 0x36, 0x00, 0x00 }, // ;
 { 0x00, 0x08, 0x14, 0x22, 0x41 }, // <
 { 0x14, 0x14, 0x14, 0x14, 0x14 }, // =
 { 0x41, 0x22, 0x14, 0x08, 0x00 }, // >
 { 0x02, 0x01, 0x51, 0x09, 0x06 }, // ?
 { 0x32, 0x49, 0x79, 0x41, 0x3E }, // @
 { 0x7E, 0x11, 0x11, 0x11, 0x7E }, // A
 { 0x7F, 0x49, 0x49, 0x49, 0x36 }, // B
 { 0x3E, 0x41, 0x41, 0x41, 0x22 }, // C
 { 0x7F, 0x41, 0x41, 0x22, 0x1C }, // D
 { 0x7F, 0x49, 0x49, 0x49, 0x41 }, // E
 { 0x7F, 0x09, 0x09, 0x01, 0x01 }, // F
 { 0x3E, 0x41, 0x41, 0x51, 0x32 }, // G
 { 0x7F, 0x08, 0x08, 0x08, 0x7F }, // H
 { 0x00, 0x41, 0x7F, 0x41, 0x00 }, // I
 { 0x20, 0x40, 0x41, 0x3F, 0x01 }, // J
 { 0x7F, 0x08, 0x14, 0x22, 0x41 }, // K
 { 0x7F, 0x40, 0x40, 0x40, 0x40 }, // L
 { 0x7F, 0x02, 0x04, 0x02, 0x7F }, // M
 { 0x7F, 0x04, 0x08, 0x10, 0x7F }, // N
 { 0x3E, 0x41, 0x41, 0x41, 0x3E }, // O
 { 0x7F, 0x09, 0x09, 0x09, 0x06 }, // P
 { 0x3E, 0x41, 0x51, 0x21, 0x5E }, // Q
 { 0x7F, 0x09, 0x19, 0x29, 0x46 }, // R
 { 0x46, 0x49, 0x49, 0x49, 0x31 }, // S
 { 0x01, 0x01, 0x7F, 0x01, 0x01 }, // T
 { 0x3F, 0x40, 0x40, 0x40, 0x3F }, // U
 { 0x1F, 0x20, 0x40, 0x20, 0x1F }, // V
 { 0x7F, 0x20, 0x18, 0x20, 0x7F }, // W
 { 0x63, 0x14, 0x08, 0x14, 0x63 }, // X
 { 0x03, 0x04, 0x78, 0x04, 0x03 }, // Y
 { 0x61, 0x51, 0x49, 0x45, 0x43 }, // Z
 { 0x00, 0x00, 0x7F, 0x41, 0x41 }, // [
 { 0x02, 0x04, 0x08, 0x10, 0x20 }, // "\"
 { 0x41, 0x41, 0x7F, 0x00, 0x00 }, // ]
 { 0x04, 0x02, 0x01, 0x02, 0x04 }, // ^
 { 0x40, 0x40, 0x40, 0x40, 0x40 }, // _
 { 0x00, 0x01, 0x02, 0x04, 0x00 }, // `
 { 0x20, 0x54, 0x54, 0x54, 0x78 }, // a
 { 0x7F, 0x48, 0x44, 0x44, 0x38 }, // b
 { 0x38, 0x44, 0x44, 0x44, 0x20 }, // c
 { 0x38, 0x44, 0x44, 0x48, 0x7F }, // d
 { 0x38, 0x54, 0x54, 0x54, 0x18 }, // e
 { 0x08, 0x7E, 0x09, 0x01, 0x02 }, // f
 { 0x08, 0x14, 0x54, 0x54, 0x3C }, // g
 { 0x7F, 0x08, 0x04, 0x04, 0x78 }, // h
 { 0x00, 0x44, 0x7D, 0x40, 0x00 }, // i
 { 0x20, 0x40, 0x44, 0x3D, 0x00 }, // j
 { 0x00, 0x7F, 0x10, 0x28, 0x44 }, // k
 { 0x00, 0x41, 0x7F, 0x40, 0x00 }, // l
 { 0x7C, 0x04, 0x18, 0x04, 0x78 }, // m
 { 0x7C, 0x08, 0x04, 0x04, 0x78 }, // n
 { 0x38, 0x44, 0x44, 0x44, 0x38 }, // o
 { 0x7C, 0x14, 0x14, 0x14, 0x08 }, // p
 { 0x08, 0x14, 0x14, 0x18, 0x7C }, // q
 { 0x7C, 0x08, 0x04, 0x04, 0x08 }, // r
 { 0x48, 0x54, 0x54, 0x54, 0x20 }, // s
 { 0x04, 0x3F, 0x44, 0x40, 0x20 }, // t
 { 0x3C, 0x40, 0x40, 0x20, 0x7C }, // u
 { 0x1C, 0x20, 0x40, 0x20, 0x1C }, // v
 { 0x3C, 0x40, 0x30, 0x40, 0x3C }, // w
 { 0x44, 0x28, 0x10, 0x28, 0x44 }, // x
 { 0x0C, 0x50, 0x50, 0x50, 0x3C }, // y
 { 0x44, 0x64, 0x54, 0x4C, 0x44 }, // z
 { 0x00, 0x08, 0x36, 0x41, 0x00 }, // {
 { 0x00, 0x00, 0x7F, 0x00, 0x00 }, // |
 { 0x00, 0x41, 0x36, 0x08, 0x00 }, // }
 { 0x08, 0x08, 0x2A, 0x1C, 0x08 }, // ->
 { 0x08, 0x1C, 0x2A, 0x08, 0x08 }, // <-
};
// ---------------------------------------------------------------------------
// MISC ROUTINES
void SetupPorts()
{
 DDRB = 0xFF; // 0010.1111; set B0-B3, B5 as outputs
 DDRC = 0x00; // 0000.0000; set PORTC as inputs
 SetBit(PORTB,0); // start with TFT reset line inactive high
}
void msDelay(int delay) // put into a routine
{ // to remove code inlining
 for (int i=0;i<delay;i++) // at cost of timing accuracy
 _delay_ms(1);
}

unsigned long intsqrt(unsigned long val)
// calculate integer value of square root
{
 unsigned long mulMask = 0x0008000;
 unsigned long retVal = 0;
 if (val > 0)
 {
 while (mulMask != 0)
 {
 retVal |= mulMask;
 if ((retVal*retVal) > val)
 retVal &= ~ mulMask;
 mulMask >>= 1;
 }
 }
 return retVal;
}
// ---------------------------------------------------------------------------
// SPI ROUTINES
//
// b7 b6 b5 b4 b3 b2 b1 b0
// SPCR: SPIE SPE DORD MSTR CPOL CPHA SPR1 SPR0
// 0 1 0 1 . 0 0 0 1
//
// SPIE - enable SPI interrupt
// SPE - enable SPI
// DORD - 0=MSB first, 1=LSB first
// MSTR - 0=slave, 1=master
// CPOL - 0=clock starts low, 1=clock starts high
// CPHA - 0=read on rising-edge, 1=read on falling-edge
// SPRx - 00=osc/4, 01=osc/16, 10=osc/64, 11=osc/128
//
// SPCR = 0x50: SPI enabled as Master, mode 0, at 16/4 = 4 MHz
void OpenSPI()
{
 SPCR = 0x50; // SPI enabled as Master, Mode0 at 4 MHz
 SetBit(SPSR,SPI2X); // double the SPI rate: 4-->8 MHz
}
void CloseSPI()
{
 SPCR = 0x00; // clear SPI enable bit
}
byte Xfer(byte data)
{
 SPDR = data; // initiate transfer
 while (!(SPSR & 0x80)); // wait for transfer to complete
 return SPDR;
}
// ---------------------------------------------------------------------------
// ST7735 ROUTINES
#define SWRESET 0x01 // software reset
#define SLPOUT 0x11 // sleep out
#define DISPOFF 0x28 // display off
#define DISPON 0x29 // display on
#define CASET 0x2A // column address set
#define RASET 0x2B // row address set
#define RAMWR 0x2C // RAM write
#define MADCTL 0x36 // axis control
#define COLMOD 0x3A // color mode
// 1.8" TFT display constants
#define XSIZE 128
#define YSIZE 160
#define XMAX XSIZE-1
#define YMAX YSIZE-1
// Color constants
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x0400
#define LIME 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
void WriteCmd (byte cmd)
{
 ClearBit(PORTB,1); // B1=DC; 0=command, 1=data
 Xfer(cmd);
 SetBit(PORTB,1); // return DC high
}
void WriteByte (byte b)
{
 Xfer(b);
}
void WriteWord (int w)
{
 Xfer(w >> 8); // write upper 8 bits
 Xfer(w & 0xFF); // write lower 8 bits
}
void Write888 (long data, int count)
{
 byte red = data>>16; // red = upper 8 bits
 byte green = (data>>8) & 0xFF; // green = middle 8 bits
 byte blue = data & 0xFF; // blue = lower 8 bits
 for (;count>0;count--)
 {
 WriteByte(red);
 WriteByte(green);
 WriteByte(blue);
 }
}
void Write565 (int data, unsigned int count)
// send 16-bit pixel data to the controller
// note: inlined spi xfer for optimization
{
 WriteCmd(RAMWR);
 for (;count>0;count--)
 {
 SPDR = (data >> 8); // write hi byte
 while (!(SPSR & 0x80)); // wait for transfer to complete
 SPDR = (data & 0xFF); // write lo byte
 while (!(SPSR & 0x80)); // wait for transfer to complete
 }
}
void HardwareReset()
{
 ClearBit(PORTB,0); // pull PB0 (digital 8) low
 msDelay(1); // 1mS is enough
 SetBit(PORTB,0); // return PB0 high
 msDelay(150); // wait 150mS for reset to finish
}
void InitDisplay()
{
 HardwareReset(); // initialize display controller
 WriteCmd(SLPOUT); // take display out of sleep mode
 msDelay(150); // wait 150mS for TFT driver circuits
 WriteCmd(COLMOD); // select color mode:
 WriteByte(0x05); // mode 5 = 16bit pixels (RGB565)
 WriteCmd(DISPON); // turn display on!
}
void SetAddrWindow(byte x0, byte y0, byte x1, byte y1)
{
 WriteCmd(CASET); // set column range (x0,x1)
 WriteWord(x0);
 WriteWord(x1);
 WriteCmd(RASET); // set row range (y0,y1)
 WriteWord(y0);
 WriteWord(y1);
}
void ClearScreen()
{
 SetAddrWindow(0,0,XMAX,YMAX); // set window to entire display
 WriteCmd(RAMWR);
 for (unsigned int i=40960;i>0;--i) // byte count = 128*160*2
 {
 SPDR = 0; // initiate transfer of 0x00
 while (!(SPSR & 0x80)); // wait for xfer to finish
 }
}
// ---------------------------------------------------------------------------
// SIMPLE GRAPHICS ROUTINES
//
// note: many routines have byte parameters, to save space,
// but these can easily be changed to int params for larger displays.
void DrawPixel (byte x, byte y, int color)
{
 SetAddrWindow(x,y,x,y);
 Write565(color,1);
}
void HLine (byte x0, byte x1, byte y, int color)
// draws a horizontal line in given color
{
 byte width = x1-x0+1;
 SetAddrWindow(x0,y,x1,y);
 Write565(color,width);
}
void VLine (byte x, byte y0, byte y1, int color)
// draws a vertical line in given color
{
 byte height = y1-y0+1;
 SetAddrWindow(x,y0,x,y1);
 Write565(color,height);
}
void Line (int x0, int y0, int x1, int y1, int color)
// an elegant implementation of the Bresenham algorithm
{
 int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
 int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
 int err = (dx>dy ? dx : -dy)/2, e2;
 for(;;)
 {
 DrawPixel(x0,y0,color);
 if (x0==x1 && y0==y1) break;
 e2 = err;
 if (e2 >-dx) { err -= dy; x0 += sx; }
 if (e2 < dy) { err += dx; y0 += sy; }
 }
}
void DrawRect (byte x0, byte y0, byte x1, byte y1, int color)
// draws a rectangle in given color
{
 HLine(x0,x1,y0,color);
 HLine(x0,x1,y1,color);
 VLine(x0,y0,y1,color);
 VLine(x1,y0,y1,color);
}
void FillRect (byte x0, byte y0, byte x1, byte y1, int color)
{
 byte width = x1-x0+1;
 byte height = y1-y0+1;
 SetAddrWindow(x0,y0,x1,y1);
 Write565(color,width*height);
}
void CircleQuadrant (byte xPos, byte yPos, byte radius, byte quad, int color)
// draws circle quadrant(s) centered at x,y with given radius & color
// quad is a bit-encoded representation of which cartesian quadrant(s) to draw.
// Remember that the y axis on our display is 'upside down':
// bit 0: draw quadrant I (lower right)
// bit 1: draw quadrant IV (upper right)
// bit 2: draw quadrant II (lower left)
// bit 3: draw quadrant III (upper left)
{
 int x, xEnd = (707*radius)/1000 + 1;
 for (x=0; x<xEnd; x++)
 {
 byte y = intsqrt(radius*radius - x*x);
 if (quad & 0x01)
 {
 DrawPixel(xPos+x,yPos+y,color); // lower right
 DrawPixel(xPos+y,yPos+x,color);
 }
 if (quad & 0x02)
 {
 DrawPixel(xPos+x,yPos-y,color); // upper right
 DrawPixel(xPos+y,yPos-x,color);
 }
 if (quad & 0x04)
 {
 DrawPixel(xPos-x,yPos+y,color); // lower left
 DrawPixel(xPos-y,yPos+x,color);
 }
 if (quad & 0x08)
 {
 DrawPixel(xPos-x,yPos-y,color); // upper left
 DrawPixel(xPos-y,yPos-x,color);
 }
 }
}
void Circle (byte xPos, byte yPos, byte radius, int color)
// draws circle at x,y with given radius & color
{
 CircleQuadrant(xPos,yPos,radius,0x0F,color); // do all 4 quadrants
}

void FillCircle (byte xPos, byte yPos, byte radius, int color)
// draws filled circle at x,y with given radius & color
{
 long r2 = radius * radius;
 for (int x=0; x<=radius; x++)
 {
 byte y = intsqrt(r2-x*x);
 byte y0 = yPos-y;
 byte y1 = yPos+y;
 VLine(xPos+x,y0,y1,color);
 VLine(xPos-x,y0,y1,color);
 }
}
// ---------------------------------------------------------------------------
// TEXT ROUTINES
//
// Each ASCII character is 5x7, with one pixel space between characters
// So, character width = 6 pixels & character height = 8 pixels.
//
// In portrait mode:
// Display width = 128 pixels, so there are 21 chars/row (21x6 = 126).
// Display height = 160 pixels, so there are 20 rows (20x8 = 160).
// Total number of characters in portait mode = 21 x 20 = 420.
//
// In landscape mode:
// Display width is 160, so there are 26 chars/row (26x6 = 156).
// Display height is 128, so there are 16 rows (16x8 = 128).
// Total number of characters in landscape mode = 26x16 = 416.
byte curX,curY; // current x & y cursor position
void GotoXY (byte x,byte y)
// position cursor on character x,y grid, where 0<x<20, 0<y<19.
{
 curX = x;
 curY = y;
}
void GotoLine(byte y)
// position character cursor to start of line y, where 0<y<19.
{
 curX = 0;
 curY = y;
}
void AdvanceCursor()
// moves character cursor to next position, assuming portrait orientation
{
 curX++; // advance x position
 if (curX>20) // beyond right margin?
 {
 curY++; // go to next line
 curX = 0; // at left margin
 }
 if (curY>19) // beyond bottom margin?
 curY = 0; // start at top again
}
void SetOrientation(int degrees)
// Set the display orientation to 0,90,180,or 270 degrees
{
 byte arg;
 switch (degrees)
 {
 case 90: arg = 0x60; break;
 case 180: arg = 0xC0; break;
 case 270: arg = 0xA0; break;
 default: arg = 0x00; break;
 }
 WriteCmd(MADCTL);
 WriteByte(arg);
}
void PutCh (char ch, byte x, byte y, int color)
// write ch to display X,Y coordinates using ASCII 5x7 font
{
 int pixel;
 byte row, col, bit, data, mask = 0x01;
 SetAddrWindow(x,y,x+4,y+6);
 WriteCmd(RAMWR);
 for (row=0; row<7;row++)
 {
 for (col=0; col<5;col++)
 {
 data = pgm_read_byte(&(FONT_CHARS[ch-32][col]));
 bit = data & mask;
 if (bit==0) pixel=BLACK;
 else pixel=color;
 WriteWord(pixel);
 }
 mask <<= 1;
 }
}
void WriteChar(char ch, int color)
// writes character to display at current cursor position.
{
 PutCh(ch,curX*6, curY*8, color);
 AdvanceCursor();
}
void WriteString(char *text, int color)
// writes string to display at current cursor position.
{
 for (;*text;text++) // for all non-nul chars
 WriteChar(*text,color); // write the char
}
void WriteInt(int i)
// writes integer i at current cursor position
{
 char str[8]; // buffer for string result
 itoa(i,str,10); // convert to string, base 10
 WriteString(str,WHITE);
}
void WriteHex(int i)
// writes hexadecimal value of integer i at current cursor position
{
 char str[8]; // buffer for string result
 itoa(i,str,16); // convert to base 16 (hex)
 WriteString(str,WHITE);
}
// ---------------------------------------------------------------------------
// TEST ROUTINES
void PixelTest()
// draws 4000 pixels on the screen
{
 for (int i=4000; i>0; i--) // do a whole bunch:
 {
 int x = rand() % XMAX; // random x coordinate
 int y = rand() % YMAX; // random y coordinate
 DrawPixel(x,y,YELLOW); // draw pixel at x,y
 }
}
void LineTest()
// sweeps Line routine through all four quadrants.	
{
 ClearScreen();
 int x,y,x0=64,y0=80;
 for (x=0;x<XMAX;x+=2) Line(x0,y0,x,0,YELLOW);
 for (y=0;y<YMAX;y+=2) Line(x0,y0,XMAX,y,CYAN);
 for (x=XMAX;x>0;x-=2) Line(x0,y0,x,YMAX,YELLOW);
 for (y=YMAX;y>0;y-=2) Line(x0,y0,0,y,CYAN);
 msDelay(2000);
}
void CircleTest()
// draw series of concentric circles
{
 for(int radius=6;radius<60;radius+=2)
 Circle(60,80,radius,YELLOW);
}
void PortraitChars()
// Writes 420 characters (5x7) to screen in portrait mode	
{
 ClearScreen();
 for (int i=420;i>0;i--)
 {
 byte x= i % 21;
 byte y= i / 21;
 char ascii = (i % 96)+32;
 PutCh(ascii,x*6,y*8,CYAN);
 }
 msDelay(2000);
}

void MoveRight()
{
	for(unsigned char i = 15; i <= 135; i = i + 5)
	{
		//ClearScreen();
		FillCircle (i, 70, 15, RED);
		msDelay(100);
		FillCircle (i, 70, 15, BLACK);
		//msDelay(100);
	}
}

//Initialize the ADC
void ADC_init()
{
	ADMUX = (1<<REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

//Read from the ADC channel ch
uint16_t ReadADC(uint8_t ch)
{
	//Select ADC Channel
	ch &= 0x07;
	ADMUX = (ADMUX & 0xF8) | ch;

	//Start Single conversion
	ADCSRA |= (1 << ADSC);

	//Wait for conversion to complete
	while(ADCSRA & (1 << ADSC));

	//Clear ADIF
	ADCSRA |= (1<<ADIF);
	return (ADC);
}
//handle up-down movement
void moveU_D(int16_t U_D)
{
	u_char tmp = y;
	if(U_D > 0)
	{
		tmp = (tmp == 7) ? 0 : tmp-1;
	}
	else if(U_D < 0)
	{
		tmp = (tmp == 0) ? 7 : tmp+1;
	}
	y = tmp;
}

//handle left-right movement
void moveL_R(int16_t L_R)
{
	u_char tmp = x;
	if(L_R < 0)
	{
		tmp = (tmp == 4) ? 0 : tmp-1;
	}
	else if(L_R > 0)
	{
		tmp = (tmp == 0) ? 4 : tmp+1;
	}
	x = tmp;
}

//determines horizontal cursor movement from joystick input
enum CPH_States {CPH_Wait, CPH_Read, CPH_Stay, CPH_Move, CPH_Ack} CP_state;
int TickFct_CursorPos_H()
{
	L_R = ReadADC(2);
	L_R -= 512;
	switch(CP_state)//Transitions
	{
		case CPH_Stay:
		if((L_R <= 250) && (L_R > -249))
		{
			CP_state = CPH_Stay;
		}
		else
		{
			CP_state = CPH_Move;
		}
		break;
		case CPH_Move:
		if((L_R <= 250) && (L_R > -249))
		{
			CP_state = CPH_Stay;
		}
		else
		{
			CP_state = CPH_Move;
		}
		break;
		default:
		CP_state = CPH_Stay;
		break;
	}
	switch(CP_state)//Actions
	{
		case CPH_Stay:
		//cursor_blink = 0;
		break;
		case CPH_Move:
		moveL_R(L_R);
		//cursor_blink = 1;
		//cursor_on = 1;
		break;
	}
	//return state;
}

//determines vertical cursor movement from joystick input
enum CPV_States {CPV_Stay, CPV_Move} CV_state;
int TickFct_CursorPos_V()
{
	U_D = ReadADC(3);
	U_D -= 512;
	switch(CV_state)//Transitions
	{
		case CPV_Stay:
		if((U_D <= 250) && (U_D > -249))
		{
			CV_state = CPV_Stay;
		}
		else
		{
			CV_state = CPV_Move;
		}
		break;
		case CPV_Move:
		if((U_D <= 250) && (U_D > -249))
		{
			CV_state = CPV_Stay;
		}
		else
		{
			CV_state = CPV_Move;
		}
		break;
		default:
		CV_state = CPV_Stay;
		break;
	}
	switch(CV_state)//Actions
	{
		case CPV_Stay:
		//cursor_blink = 0;
		break;
		case CPV_Move:
		moveU_D(U_D);
		//cursor_blink = 1;
		//cursor_on = 1;
		break;
	}
	//return state;
}
// ---------------------------------------------------------------------------
// MAIN PROGRAM
int main()
{
	DDRA = 0x83; PORTA = 0x7C;
	TimerSet(10);
	TimerOn();
	
	ADC_init();
	
 SetupPorts(); // use PortB for LCD interface
 OpenSPI(); // start communication to TFT
 InitDisplay(); // initialize TFT controller
  ClearScreen();
 SetOrientation(90);
 
 CV_state = CPV_Stay;
 CP_state = CPH_Stay;
 
  while (1) {
	  TickFct_CursorPos_H();
	  TickFct_CursorPos_V();
	  FillCircle (x, y, 15, RED);
	  msDelay(100);
	  FillCircle (x, y, 15, BLACK);
	  while (!TimerFlag){}   // Wait for timer period
	  TimerFlag = 0;         // Lower flag raised by timer
  }
  
 //FillCircle (x, y, 15, RED);
 //MoveRight();
 
 CloseSPI(); // close communication with TFT
}