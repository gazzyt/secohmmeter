#ifndef EEPROM_H
#define EEPROM_H

struct calibration_values_type
{
	uint f1;
	uint fcal;
	float polarCDivisor;
};

void ReadCalibrationValues(struct calibration_values_type *values);
void WriteCalibrationValues(const struct calibration_values_type *values);
void EraseCalibrationValues();

#endif
