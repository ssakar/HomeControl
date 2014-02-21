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

#ifndef SWITCH_H
#define SWITCH_H

const int SWITCH_NAME_SIZE = 10;

class Switch {
 	unsigned int group : 5;
 	unsigned int device : 5;
	
	bool on : 1;
	bool active : 1;
	
	char name[SWITCH_NAME_SIZE+1];
public:
	Switch();

	void setGroup(const char*);
	void setDevice(const char*);

	char* getGroup() const;
	char* getDevice() const;
	
	bool isOn() const;
	void setOn(bool);

	bool isActive() const;
	void setActive(bool);
	
	void setName(const char*);
	const char* getName() const;
};

#endif