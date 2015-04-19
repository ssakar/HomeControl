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

#ifndef DATE_TIME_H
#define DATE_TIME_H

#include "Arduino.h"
#include <Printable.h>


typedef unsigned long time_t;

struct Week {
	byte sun : 1;
	byte mon : 1;
	byte tue : 1;
	byte wed : 1;
	byte thu : 1;
	byte fri : 1;
	byte sat : 1;
	byte all : 1;
};

typedef union {
	Week week;
	byte days;
} Week_t;

class DateTime : public Printable {	
	byte second; 	// 0-59
	byte minute; 	// 0-59
	byte hour; 		// 0-23
	byte dow;		// 1-7, sunday is day 1
	byte day;		// 1 is first day 
	byte month;		// 1 is january 
	int year;		// e.g. 2014

	time_t toUnix(const DateTime&) const;
	size_t printFormat(Print&, int, const char*) const;
	size_t printDayOfWeek(Print&) const;
public:
	DateTime();
	DateTime(time_t);
	DateTime(byte, byte, byte, byte, byte, byte, int);
	
	void setSecond(byte);
	void setMinute(byte);
	void setHour(byte);
	void setDayOfWeek(byte);
	void setDay(byte);
	void setMonth(byte);
	void setYear(int);
	
	byte getSecond() const;
	byte getMinute() const;
	byte getHour() const;
	byte getDayOfWeek() const;
	byte getDay() const;
	byte getMonth() const;
	int getYear() const;
	time_t getUnix() const;	
	bool isSameDay(Week_t);

	bool operator<(const DateTime&) const;	
	
	virtual size_t printTo(Print&) const;
};

#endif
