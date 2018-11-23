#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>
#include <mcs51/at89x52.h>

#include "common.h"
#include "display.h"
#include "eeprom.h"

__sbit __at(0xB2) polar_c_charged;	// The MCU pin connected to U3B output
__sbit __at(0xA0) lx_button;		// The MCU pin that indicates whether the LX button is pressed
__sbit __at(0xA1) cx_button;		// The MCU pin that indicates whether the CX button is pressed
__sbit __at(0xA6) fx_button;		// The MCU pin that indicates whether the FX button is pressed
__sbit __at(0xA7) small_c_button;	// The MCU pin that indicates whether the Small_C button is pressed
__sbit __at(0xB6) big_c_button;		// The MCU pin that indicates whether the Bid_C button is pressed
__sbit __at(0xA5) discharge_switch;	// The MCU pin connected to Q1 base that causes the polar C discharge

typedef enum
{
	NONE = 0,
	MULTIPLE,
	FX,
	CX,
	LX,
	MIN_C,
	MAX_C,
	CALIBRATE_REQUEST,
	CALIBRATE_CONFIRM
} button_state_type;

typedef enum
{
	MODE_NONE = 0,
	MODE_LC,
	MODE_POLAR_C
} operating_mode;


uint ReadNextPeriodCycles();
void WaitPeriod(uchar numPeriods);
button_state_type ReadButtons();
void CalibrateSelf();
void CalibratePolarC();
ulong MeasurePolarCX();
void CheckClearCalibration();
void CalculateCX();
void CalculateLX();
void CalculateFX();
void CalculatePolarCX(bool smallC);
void DisplayLong(uchar row, uchar startCol, long number);

void SetLCMode();
void SetPolarCMode();

volatile uchar timer_cycles;		// The count of the number of times a timer has overflowed
volatile uint oscillator_cycles;	// The count of the number of cycles the RC oscillator has performed

struct calibration_values_type calibration_values;
operating_mode current_operating_mode = MODE_NONE;

const float ccal_uF = 0.001;	// The value of calibration capacitor used
__code const uchar *msg_setCalibrationCapacitor = "Set 1nF cap...";
__code const uchar *msg_setPolarCalibrationCapacitor = "Set polar cap...";
__code const uchar *msg_setPolarCalibrationCapacitorBigC = "Press Big C";
__code const uchar *msg_setPolarCalibrationCapacitorSmallC = "Press Small C";

const int acceptableDrift = 100;	// Acceptable difference between stored and measured free running frequency.
									// Any greater than this and we will recalibrate

__code const uchar *msg_calibrating = "Calibrating.....";

main()
{
	discharge_switch = 1;
	LCMInit();
	SetLCMode();
	DisplayListChar(0, 0, 0, "F/L/C GAZZYTJUDZ");
	CheckClearCalibration();

	while (ReadButtons() != NONE)
	{
		DisplayListChar(1, 0, 0, "Btns pressed!");
	}

	CalibrateSelf();

	while (1)
	{
		switch (ReadButtons())
		{
		case MULTIPLE:
		case CALIBRATE_REQUEST:
		case CALIBRATE_CONFIRM:
			DisplayListChar(1, 0, 0, "Multiple Btns");
			break;

		case NONE:
			ClearLine(1);
			break;

		case LX:
			CalculateLX();
			break;

		case CX:
			CalculateCX();
			break;

		case FX:
			CalculateFX();
			break;

		case MIN_C:
			CalculatePolarCX(true);
			break;

		case MAX_C:
			CalculatePolarCX(false);
			break;
		};
	}
}

// Measure Big_C or Small_C depending on parameter and display result on the LCD
void CalculatePolarCX(bool smallC)
{
	// In the Big_C case we are charging through a 100R resistor.
	// Thus the formula C=t/R simplifies to the point where the time
	// to charge to 62% in microseconds is equal to the capacitance
	// in hundredths of microFarads. This is exactly how we want to
	// display it so no further calulation required.
	ulong result = MeasurePolarCX();

	// If we are in Small_C mode then use the calibration value
	// to allow for the large charging resistor.
	if (smallC)
	{
		result = (ulong)((float)result / calibration_values.polarCDivisor);
	}

	DisplayListChar(1, 0, 0, "Cx=");
	DisplayOneChar(1, 3, result / 1000000 % 10 + 0x30);
	DisplayOneChar(1, 4, result / 100000 % 10 + 0x30);
	DisplayOneChar(1, 5, result / 10000 % 10 + 0x30);
	DisplayOneChar(1, 6, result / 1000 % 10 + 0x30);
	DisplayOneChar(1, 7, result / 100 % 10 + 0x30);
	DisplayOneChar(1, 8, '.');
	DisplayOneChar(1, 9, result / 10 % 10 + 0x30);
	DisplayOneChar(1, 10, result % 10 + 0x30);
	DisplayListChar(1, 11, 0, "uF  ");
}

// Return the number of microseconds taken to charge a polarised capacitor 
// from discharged to the threshold set by R16
ulong MeasurePolarCX()
{
	ulong result = 0;
	TR0 = 0;
	TR1 = 0;
	timer_cycles = 0;

	// Discharge the capacitor under test
	discharge_switch = 0;
	delayms(250);
	discharge_switch = 1;

	SetPolarCMode();

	// Start cycle count at 0
	timer_cycles = 0;
	TH1=0;
	TL1=0;

	// Start Timer 1
	TR1 = 1;

	// Wait until the capacitor under test charges up to the threshold voltage
	polar_c_charged = 1;

	while (polar_c_charged == 1)
		;

	// Stop Timer 1
	TR1 = 0;

	result = timer_cycles * 65536UL;

	result += TH1 * 256UL + TL1;

	return result;
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
	if (current_operating_mode != MODE_LC)
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

		current_operating_mode = MODE_LC;
	}
}

void SetPolarCMode()
{
	if (current_operating_mode != MODE_POLAR_C)
	{
		// Timer 1 - mode 1, timer
		TMOD = 0x10;

		TH1=0;
		TL1=0;

		// Global interrupt enable
		EA = 1;

		// Enable Timer 1 interrupt
		ET1 = 1;

		current_operating_mode = MODE_POLAR_C;
	}
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

/*
 * Check which button(s) are pressed.
 * When a button is pressed, it pulls its input pin LOW.
 */
button_state_type ReadButtons()
{
	int pressed_button_count = lx_button + cx_button + fx_button + small_c_button + big_c_button;

	if (pressed_button_count == 5)
		return NONE;

	// Check for button combos relating to clearing the calibration values
	if (lx_button == 0 && cx_button == 0)
	{
		if (pressed_button_count == 3)
		{
			return CALIBRATE_REQUEST;
		}

		if (pressed_button_count == 2 && fx_button == 0)
		{
			return CALIBRATE_CONFIRM;
		}
	}

	if (pressed_button_count < 4)
		return MULTIPLE;

	// At this point we know there is exactly one button pressed
	if (lx_button == 0)
		return LX;

	if (cx_button == 0)
		return CX;

	if (fx_button == 0)
		return FX;

	if (small_c_button == 0)
		return MIN_C;

	if (big_c_button == 0)
		return MAX_C;

	return NONE;
}

void CalibrateSelf()
{
	uint f1;
	ReadCalibrationValues(&calibration_values);

#ifdef DEBUG
	DisplayLong(0, 0, calibration_values.f1);
	DisplayLong(1, 0, calibration_values.fcal);
	WaitPeriod(40);
#endif

	DisplayListChar(0, 0, 0, msg_calibrating);
	WaitPeriod(10);
	f1 = ReadNextPeriodCycles();

	if (abs((int)f1 - (int)calibration_values.f1) > acceptableDrift)
	{
		DisplayListChar(0, 0, 0, msg_setCalibrationCapacitor);

		while (cx_button == 1)
			;

		DisplayListChar(0, 0, 0, msg_calibrating);
		WaitPeriod(10);
		calibration_values.fcal = ReadNextPeriodCycles();
		calibration_values.f1 = f1;

		CalibratePolarC();

		WriteCalibrationValues(&calibration_values);
	}

	ClearLine(0);
}

void CalibratePolarC()
{
	ulong bigC, smallC;

	DisplayListChar(0, 0, 0, msg_setPolarCalibrationCapacitor);
	DisplayListChar(1, 0, 0, msg_setPolarCalibrationCapacitorBigC);

	while (big_c_button == 1)
		;

	bigC = MeasurePolarCX();

	DisplayListChar(1, 0, 0, msg_setPolarCalibrationCapacitorSmallC);

	while (small_c_button == 1)
		;

	smallC = MeasurePolarCX();

	calibration_values.polarCDivisor = (float)smallC / (float)bigC;
}

/*
 * Check if the clear calibration key combo was pressed and, if confirmed,
 * clear the stored calibration from eeprom
 */
void CheckClearCalibration()
{
	if (ReadButtons() == CALIBRATE_REQUEST)
	{
		DisplayListChar(0, 0, 0, "Press FX btn    ");
		DisplayListChar(1, 0, 0, "to clear calibr.");

		while (ReadButtons() == CALIBRATE_REQUEST)
			;

		WaitPeriod(10);

		if (ReadButtons() == CALIBRATE_CONFIRM)
		{
			// Clear the stored calibration values
			EraseCalibrationValues();
		}

		ClearScreen();
	}
}

// Measure CX and display result on the LCD
void CalculateCX()
{
	uint cycles;
	float numerator;
	float denominator;
	long result;

	SetLCMode();

	WaitPeriod(10);
	cycles = ReadNextPeriodCycles();

	numerator = __uint2fs(calibration_values.f1) / __uint2fs(cycles);
	numerator = numerator * numerator - 1.0;

	denominator = __uint2fs(calibration_values.f1) / __uint2fs(calibration_values.fcal);
	denominator = denominator * denominator - 1.0;

	result = __fs2slong(numerator * ccal_uF / denominator * 1000000.0);

	DisplayListChar(1, 0, 0, "Cx=");
	DisplayLong(1, 3, result);
	DisplayListChar(1, 10, 0, "pF    ");
}

// Measure LX and display result on the LCD
void CalculateLX()
{
	uint cycles;
	float a, b, d;
	long result;

	SetLCMode();

	WaitPeriod(10);
	cycles = ReadNextPeriodCycles();

	a = __uint2fs(calibration_values.f1) / __uint2fs(cycles);
	a = a * a - 1.0;

	b = __uint2fs(calibration_values.f1) / __uint2fs(calibration_values.fcal);
	b = b * b - 1.0;

	d = 40.0 * PI * __uint2fs(calibration_values.f1);
	d = (1.0 / d) * (1.0 / d);

	result = __fs2slong((a * b * d * 1000000.0) / (ccal_uF / 1000000.0));

	DisplayListChar(1, 0, 0, "Lx=");
	DisplayLong(1, 3, result);
	DisplayListChar(1, 10, 0, "uH    ");
}

// Calculate the frequency of input if the f button is pressed
// We run 20 cycles of 50ms each giving us the frequency directly which we store in result
void CalculateFX()
{
	long result = 0L;
	uchar i;

	SetLCMode();

	WaitPeriod(10);
	for (i = 0; i < 20; ++i)
	{
		result += ReadNextPeriodCycles();
	}

	DisplayListChar(1, 0, 0, "Fx=");
	DisplayLong(1, 3, result);
	DisplayListChar(1, 10, 0, "Hz    ");
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
