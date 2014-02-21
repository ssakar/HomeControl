/*
	HomeControl
	Copyright (C) 2014 Serkan Sakar <ssakar@gmx.de>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Arduino.h"
#include "TempSensor.h"


TempSensor::TempSensor(int pin)
	:ds(pin)
{}

float TempSensor::read()
{
	byte data[12];
	byte addr[8];

	if (!ds.search(addr)) {
		ds.reset_search();
		return ERROR;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1);	// start converting
	ds.reset();
	ds.select(addr);
	ds.write(0xBE);		//start sending data

	for (int i = 0; i < 9; i++)
		data[i] = ds.read();

	ds.reset_search();

	byte MSB = data[1];
	byte LSB = data[0];

	float raw = ((MSB << 8) | LSB);
	float realTempC = raw / 16;
	return realTempC;
}

const char* TempSensor::getName() const
{
	static const char name[] = "DS18B20";
	return name;
}
