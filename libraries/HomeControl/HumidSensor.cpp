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

#include "HumidSensor.h"


HumidSensor::HumidSensor(byte _pin)
	:dht11(_pin, DHT11)
{
	dht11.begin();
}

float HumidSensor::read()
{
	return dht11.readHumidity();
}

const char* HumidSensor::getName() const
{
	static const char name[] = "DHT11";
	return name;
}
