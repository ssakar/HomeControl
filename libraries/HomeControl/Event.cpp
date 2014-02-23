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

#include "Event.h"


Event::Event():
	id(0), time(0)
{}

Event::Event(unsigned long _id):
	id(_id), time(0)
{}

void Event::setId(unsigned long _id)
{
	id = _id;
}

void Event::setTime(time_t t)
{
	time = t;
}

unsigned long Event::getId() const
{
	return id;
}

time_t Event::getTime() const
{
	return time;
}


EventRule::EventRule():
	eventId(255), switchId(255), on(0), active(0), inv(0)
{
	memcpy(name, 0, sizeof(name));
}

void EventRule::setEventId(unsigned long id)
{
	eventId = id;
}

void EventRule::setSwitchId(byte id)
{
	switchId = id;
}

bool EventRule::turnOn() const
{
	return on;
}

void EventRule::setOn(bool _on)
{
	on = _on;
}

void EventRule::setToggle(bool toggle)
{
	inv = toggle;
}

unsigned long EventRule::getEventId() const
{
	return eventId;
}

byte EventRule::getSwitchId() const
{
	return switchId;
}

bool EventRule::toggle() const
{
	return inv;
}

bool EventRule::isActive() const
{
	return active;
}

void EventRule::setActive(bool act)
{
	active = act;
}

void EventRule::setName(const char* _name)
{
	strncpy(name, _name, sizeof(name));
	name[RULE_NAME_SIZE] = '\0';
}

const char* EventRule::getName() const
{
	return name;
}
