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

#ifndef UTIL_H
#define UTIL_H


//#define DEBUG

template<class T>
inline Print& operator<<(Print &p, T rhs)
{
	p.print(rhs); 
	return p; 
}

int freeRam() 
{
	extern int __heap_start, *__brkval; 
	int v; 
	return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval); 
}

#ifdef DEBUG
#define DEBUG_PRINT(str) \
	Serial.print(millis()); \
	Serial.print(": "); \
	Serial.print(__PRETTY_FUNCTION__); \
	Serial.print(' '); \
	Serial.print(__FILE__); \
	Serial.print(':'); \
	Serial.print(__LINE__); \
	Serial.print(' '); \
	Serial.println(str);
#else
#define DEBUG_PRINT(str)
#endif

#define GCC_VERSION (__GNUC__ * 10000 \
	+ __GNUC_MINOR__ * 100 \
	+ __GNUC_PATCHLEVEL__)


#endif
