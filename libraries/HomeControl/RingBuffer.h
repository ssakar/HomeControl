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

#ifndef RING_BUFFER_H
#define RING_BUFFER_H


template<class T, byte sz> class RingBuffer {
	byte start;
	byte count;
	T data[sz];
public:
	RingBuffer();
	
	void put(const T);
	T& get();
	void clear();
	
	bool isFull() const;
	bool isEmpty() const;
	
	byte getSize() const;
	
	const T& operator[](byte) const;
	T& operator[](byte);
};

template<class T, byte sz> 
RingBuffer<T, sz>::RingBuffer()
{
	clear();
}

template<class T, byte sz> 
void RingBuffer<T, sz>::put(const T elem)
{
	int i = (start + count) % sz;
	data[i] = elem;
	
	if (count == sz)
		start = (start + 1) % sz;
	else
		count++;
}

template<class T, byte sz> 
T& RingBuffer<T, sz>::get()
{
	int i = start;
	start = (start + 1) % sz;
	count--;
	return data[i];
}

template<class T, byte sz> 
bool RingBuffer<T, sz>::isFull() const
{
	return count == sz;
}

template<class T, byte sz> 
bool RingBuffer<T, sz>::isEmpty() const
{
	return count == 0;
}

template<class T, byte sz> 
void RingBuffer<T, sz>::clear()
{
	start = count = 0;
}

template<class T, byte sz> 
byte RingBuffer<T, sz>::getSize() const
{
	return count;
}

template<class T, byte sz> 
const T& RingBuffer<T, sz>::operator[](byte i) const
{
	return data[(start + i) % sz];
}

template<class T, byte sz> 
T& RingBuffer<T, sz>::operator[](byte i)
{
	return data[(start + i) % sz];
}

#endif