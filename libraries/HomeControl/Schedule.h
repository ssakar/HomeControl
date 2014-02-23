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

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "Arduino.h"
#include "DateTime.h"

const int SCHEDULE_NAME_SIZE = 10;


class Schedule {
	time_t time; 
	time_t duration;
	Week_t w;

	bool on 	: 1;
	bool active : 1;

	byte switchId;
	byte sensorId;
	float threshold;
	
	char name[SCHEDULE_NAME_SIZE+1];
public:
	Schedule();
	
	time_t getTime() const;
	Week_t getDays() const;
	time_t getDuration() const;
	
	void setTime(time_t);
	void setDays(Week_t);
	void setDuration(time_t);
	
	void setSwitchId(byte);
	void setSensorId(byte);
	
	byte getSwitchId() const;
	byte getSensorId() const;

	float getThreshold() const;
	void setThreshold(float);
	
	bool turnOn() const;
	void setOn(bool);

	bool isActive() const;
	void setActive(bool);
	
	void setName(const char*);
	const char* getName() const;
};

#endif