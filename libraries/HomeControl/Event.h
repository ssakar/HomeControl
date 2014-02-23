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

#ifndef EVENT_H
#define EVENT_H


#include "Arduino.h"
#include "Time.h"

const int RULE_NAME_SIZE = 10;


class Event {
	unsigned long id;
	time_t time;
public:
	Event();
	Event(unsigned long);

	void setId(unsigned long);
	void setTime(time_t);

	unsigned long getId() const;
	time_t getTime() const;
};

class EventRule {
	unsigned long eventId;
	byte switchId;
	bool on		: 1;
	bool active : 1;
	bool inv	: 1;

	char name[RULE_NAME_SIZE+1];
public:
	EventRule();

	void setEventId(unsigned long);
	void setSwitchId(byte);
	void setOn(bool);
	void setToggle(bool);

	unsigned long getEventId() const;
	byte getSwitchId() const;
	bool turnOn() const;
	bool toggle() const;

	bool isActive() const;
	void setActive(bool);

	void setName(const char*);
	const char* getName() const;
};

#endif