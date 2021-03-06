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

#include "Switch.h"
#include "Arduino.h"


const int MAX_SIZE = 6;
char group_buf[MAX_SIZE] = {0};
char device_buf[MAX_SIZE] = {0};


Switch::Switch()
	:group(0), device(0), id(0), on(0), pin(0), active(0), scheduled(0)
{
	memcpy(name, 0, sizeof(name));
}

void Switch::setGroup(const char* grp)
{
	group = 0;

	for (int i = 0; i < 5; i++) {
		if (grp[i] == '1')
			bitSet(group, i);
	}
}

void Switch::setDevice(const char* dev)
{
	device = 0;

	for (int i = 0; i < 5; i++) {
		if (dev[i] == '1')
			bitSet(device, i);
	}
}

void Switch::setId(byte _id)
{
	id = _id;
}

char* Switch::getGroup() const
{
	for (int i = 0; i < 5; i++) {
		if (bitRead(group, i))
			group_buf[i] = '1';
		else
			group_buf[i] = '0';
	}
	group_buf[5] = '\0';
	return group_buf;
}

char* Switch::getDevice() const
{
	for (int i = 0; i < 5; i++) {
		if (bitRead(device, i))
			device_buf[i] = '1';
		else
			device_buf[i] = '0';
	}
	device_buf[5] = '\0';
	return device_buf;
}

byte Switch::getId() const
{
	return id;
}

bool Switch::isOn() const
{
	return on;
}

void Switch::setOn(bool _on)
{
	on = _on;
}

bool Switch::isPin() const
{
	return pin;
}

void Switch::setPin(bool _pin)
{
	pin = _pin;
}

bool Switch::isActive() const
{
	return active;
}

void Switch::setScheduled(bool sched)
{
	scheduled = sched;
}

bool Switch::isScheduled() const
{
	return scheduled;
}

void Switch::setActive(bool act)
{
	active = act;
}

void Switch::setName(const char* _name)
{
	strncpy(name, _name, sizeof(name));
	name[SWITCH_NAME_SIZE] = '\0';
}

const char* Switch::getName() const
{
	return name;	
}
