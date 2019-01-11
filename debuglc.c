#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>
#include <mcs51/at89x52.h>

#include "common.h"
#include "display.h"


uint ReadNextPeriodCycles();
void WaitPeriod(uchar numPeriods);
void DisplayLong(uchar row, uchar startCol, long number);
void DisplayCycles();

void SetLCMode();

volatile uchar timer_cycles;		// The count of the number of times a timer has overflowed
volatile uint oscillator_cycles;	// The count of the number of cycles the RC oscillator has performed

main()
{
	LCMInit();
	SetLCMode();

	while (1)
	{
		DisplayCycles();
	}
}


void timer1() __interrupt 3
{
	TH1=0;
	TL1=0;

	timer_cycles++;
}

void timer0() __interrupt 1
{
	// Set timer 0 for another 50ms
	TH0 = (65536 - 50000) / 256;
	TL0 = (65536 - 50000) % 256;

	// Increment a count of # times timer 0 has fired
	timer_cycles++;

	// Stop timer 0
	TR0 = 0;

	// Store current value of timer 1 into temp
	oscillator_cycles = TH1 * 256 + TL1;

	// Reset timer 1
	TH1 = 0;
	TL1 = 0;

	// Start timer 0
	TR0 = 1;
}

void SetLCMode()
{
	// Timer 0 - mode 1, timer
	// Timer 1 - mode 1, counter
	TMOD = 0x51;

	// Device is set in 12T mode so we get a timer tick each 12 clock cycles.
	// We use 12MHz crystal so each timer tick is 1 micro-second.
	// 50000 is thus 50ms
	TH0 = (65536 - 50000) / 256;
	TL0 = (65536 - 50000) % 256;

	// Global interrupt enable
	EA = 1;

	// Enable Timer 0 interrupt
	ET0 = 1;

	// Disable Timer 1 interrupt
	ET1 = 0;

	TH1 = 0;
	TL1 = 0;
}

// Count the number of oscillator cycles in one timer cycle (50ms)
uint ReadNextPeriodCycles()
{
	// Enable both timers
	TR0 = 1;
	TR1 = 1;

	// Start cycle count at 0
	timer_cycles = 0;

	// Wait until the end of the period
	while (timer_cycles < 1)
		;

	// Reset cycle count
	timer_cycles = 0;

	// Disable both timers
	TR0 = 0;
	TR1 = 0;

	return oscillator_cycles;
}

// Wait for a number of timer cycles (50ms)
void WaitPeriod(uchar numPeriods)
{
	// Enable both timers
	TR0 = 1;
	TR1 = 1;

	// Start period count at 0
	timer_cycles = 0;

	// Wait until enough periods have passed
	while (timer_cycles < numPeriods)
		;

	// Disable both timers
	TR0 = 0;
	TR1 = 0;
}


void DisplayCycles()
{
	uint cycles;

	cycles = ReadNextPeriodCycles();

	DisplayLong(1, 0, cycles);
}


void DisplayLong(uchar row, uchar startCol, long number)
{
	DisplayOneChar(row, startCol, number / 1000000 % 10 + 0x30);
	DisplayOneChar(row, startCol + 1, number / 100000 % 10 + 0x30);
	DisplayOneChar(row, startCol + 2, number / 10000 % 10 + 0x30);
	DisplayOneChar(row, startCol + 3, number / 1000 % 10 + 0x30);
	DisplayOneChar(row, startCol + 4, number / 100 % 10 + 0x30);
	DisplayOneChar(row, startCol + 5, number / 10 % 10 + 0x30);
	DisplayOneChar(row, startCol + 6, number % 10 + 0x30);
}
