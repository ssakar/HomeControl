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

#ifndef TIME_H
#define TIME_H

#include "Arduino.h"
#include "IPAddress.h"
#include "DateTime.h"


class Time {
	time_t lastSync;
	time_t interval;
	IPAddress ntpServer[2];
	int utc;
	
	void createNtpPacket();
	bool syncTime(const IPAddress&);
	bool writeRTC(const DateTime&);
public:
	Time();
	
	void begin();
	void syncTime(bool = false);
	void setSyncInterval(time_t);
	void setTimeServer(const IPAddress&);
	void setTime(DateTime&);
	void setOffset(int);
	
	DateTime getTime() const;
	IPAddress getTimeServer() const;
	time_t getSyncInterval() const;
	int getOffset() const;
	
	bool isRunning() const;
};

#endif