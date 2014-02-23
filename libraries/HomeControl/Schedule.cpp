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

#include "Schedule.h"


Schedule::Schedule():
	time(0), duration(0), on(0), active(0),
	switchId(255), sensorId(255), threshold(100)
{
	w.days = 0;
	memcpy(name, 0, sizeof(name));
}

time_t Schedule::getTime() const
{
	return time;
}

Week_t Schedule::getDays() const
{
	return w;
}

time_t Schedule::getDuration() const
{
	return duration;
}

void Schedule::setTime(time_t _time)
{
	time = _time;
}

void Schedule::setDays(Week_t _w)
{
	w = _w;
}

void Schedule::setDuration(time_t d)
{
	duration = d;
}

void Schedule::setSwitchId(byte id)
{
	switchId = id;
}

void Schedule::setSensorId(byte id)
{
	sensorId = id;
}

byte Schedule::getSwitchId() const
{
	return switchId;
}

byte Schedule::getSensorId() const
{
	return sensorId;
}

float Schedule::getThreshold() const
{
	return threshold;
}

void Schedule::setThreshold(float th)
{
	threshold = th;
}

bool Schedule::turnOn() const
{
	return on;
}

void Schedule::setOn(bool _on)
{
	on = _on;
}

bool Schedule::isActive() const
{
	return active;
}

void Schedule::setActive(bool act)
{
	active = act;
}

void Schedule::setName(const char* _name)
{
	strncpy(name, _name, sizeof(name));
	name[SCHEDULE_NAME_SIZE] = '\0';
}

const char* Schedule::getName() const
{
	return name;	
}
