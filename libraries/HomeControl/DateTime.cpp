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

#include "DateTime.h"


namespace {
	
	const byte monthDays[] = 
	{31,28,31,30,31,30,31,31,30,31,30,31}; 
	
	const time_t UNIX_EPOCH = 1970;
	
	const time_t SECS_PER_MIN 	= 60;
	const time_t SECS_PER_HOUR 	= 3600;
	const time_t SECS_PER_DAY 	= 86400;
	
	
	bool isLeapYear(int year) 
	{
		return !(year % 400) || year % 100 && !(year % 4);	
	}
}

DateTime::DateTime()
{}

DateTime::DateTime(byte _second, byte _minute, byte _hour, 
		byte _dow, byte _day, byte _month, int _year):
	second(_second), minute(_minute), hour(_hour), 
	dow(_dow), day(_day), month(_month), year(_year)
{}

DateTime::DateTime(time_t unix) 
{
	second = unix % 60;
	unix /= 60;
	minute = unix % 60;
	unix /= 60;
	hour = unix % 24;
	
	unsigned int d = unix / 24;
	dow = ((d + 4) % 7) + 1;
	byte leap;
	
	for (year = UNIX_EPOCH; ; year++) {
		leap = isLeapYear(year);
		if (d < 365 + leap)
			break;
		d -= 365 + leap;
	}
	
	for (month = 1; ; month++) {
		byte daysPerMonth = monthDays[month-1];
		if (leap && month == 2)
			daysPerMonth++;
		if (d < daysPerMonth)
			break;
		d -= daysPerMonth;
	}
	day = d + 1;
}

time_t DateTime::toUnix(const DateTime& dt) const
{   
	time_t unix;
	
	// add days until current year
	unix = (dt.getYear() - UNIX_EPOCH) * 365 * SECS_PER_DAY;
	for (int i = UNIX_EPOCH; i < dt.getYear(); i++) { 
		if (isLeapYear(i))
			unix += SECS_PER_DAY;
	}
	// add days of the current year, until current month
	for (int i = 1; i < dt.getMonth(); i++) {
		if (i == 2 && isLeapYear(dt.getYear()))  
			unix += SECS_PER_DAY * 29;
		else 
			unix += SECS_PER_DAY * monthDays[i-1];  
	}
	// add days of the current month
	unix += (dt.getDay()-1) * SECS_PER_DAY;
	unix += dt.getHour() * SECS_PER_HOUR;
	unix += dt.getMinute() * SECS_PER_MIN;
	unix += dt.getSecond();
	
	return unix; 
}

void DateTime::setSecond(byte _second)
{
	second = _second;
}

void DateTime::setMinute(byte _minute)
{
	minute = _minute;
}

void DateTime::setHour(byte _hour)
{
	hour = _hour;
}

void DateTime::setDayOfWeek(byte _dow)
{
	dow = _dow;
}

void DateTime::setDay(byte _day)
{
	day = _day;
}

void DateTime::setMonth(byte _month)
{
	month = _month;
}

void DateTime::setYear(int _year)
{
	year = _year;
}

byte DateTime::getSecond() const
{
	return second;
}

byte DateTime::getMinute() const
{
	return minute;
}

byte DateTime::getHour() const
{
	return hour;
}

byte DateTime::getDayOfWeek() const
{
	return dow;
}

byte DateTime::getDay() const
{
	return day;
}

byte DateTime::getMonth() const
{
	return month;
}

int DateTime::getYear() const
{
	return year;
}

time_t DateTime::getUnix() const
{
	return toUnix(*this);
}

bool DateTime::isSameDay(Week_t w)
{
	return bitSet(w.days, dow) || w.week.all;
}

bool DateTime::operator<(const DateTime& dt) const
{
	return getUnix() < dt.getUnix();
}

size_t DateTime::printTo(Print& p) const
{
	size_t n = 0;

	n += printDayOfWeek(p);

	n += printFormat(p, day, ".");
	n += printFormat(p, month, ".");
	n += printFormat(p, year, " ");

	n += printFormat(p, hour, ":");
	n += printFormat(p, minute, ":");
	n += printFormat(p, second, NULL);

	return n;
}

size_t DateTime::printDayOfWeek(Print& p) const
{
	size_t n = 0;

	switch (dow) {
	case 1:
		n += p.print("Sun");
		break;
	case 2:
		n += p.print("Mon");
		break;
	case 3:
		n += p.print("Tue");
		break;
	case 4:
		n += p.print("Wed");
		break;
	case 5:
		n += p.print("Thu");
		break;
	case 6:
		n += p.print("Fri");
		break;
	case 7:
		n += p.print("Sat");
		break;
	}
	n += p.print(" ");

	return n;
}

size_t DateTime::printFormat(Print& p, int time, const char* del) const
{
	size_t n = 0;

	if (time < 10)
		n += p.print(0);
	n += p.print(time);
	if (del)
		n += p.print(del);

	return n;
}

