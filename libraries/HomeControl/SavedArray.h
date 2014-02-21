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

#ifndef SAVED_ARRAY_H
#define SAVED_ARRAY_H

#include "Arduino.h"
#include <avr/eeprom.h>


template<class T, byte sz> class SavedArray {
	byte size;
	void* eeprom;
	T data[sz];
public:
	SavedArray(void*);
	~SavedArray();

	void save();
	void load();
	
	T& instance();
	
	T& operator[](byte) const;
	T& operator[](byte);
	
	byte getSize() const;
};

template<class T, byte sz>
SavedArray<T, sz>::SavedArray(void* ee)
	:size(sz), eeprom(ee)
{}

template<class T, byte sz>
SavedArray<T, sz>::~SavedArray()
{}

template<class T, byte sz>
T& SavedArray<T, sz>::instance()
{
	return data[0];
}

template<class T, byte sz>
T& SavedArray<T, sz>::operator[](byte i) const
{
	return data[i];
}

template<class T, byte sz>
T& SavedArray<T, sz>::operator[](byte i)
{
	return data[i];
}

template<class T, byte sz>
byte SavedArray<T, sz>::getSize() const
{
	return size;
}

template<class T, byte sz>
void SavedArray<T, sz>::save()
{
	eeprom_update_block(data, eeprom, sizeof(T)*size);	
}

template<class T, byte sz>
void SavedArray<T, sz>::load()
{
	eeprom_read_block(data, eeprom, sizeof(T)*size);	
}

#endif