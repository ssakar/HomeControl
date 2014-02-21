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

#include "Time.h"
#include <EthernetUdp.h>
#include <Wire.h>


namespace {

	const time_t SECS_PER_MIN 	= 60;
	const time_t SECS_PER_HOUR 	= 3600;
	const time_t SECS_PER_DAY 	= 86400;
	const time_t SECS_PER_70YR	= 2208988800UL;

	const int LOCAL_PORT = 8653;
	const int NTP_PORT = 123;
	const int DS1307_ADDRESS = 0x68;
	const byte PACKET_SIZE = 48;
	
	byte buffer[PACKET_SIZE];


	byte decTobcd(byte val)
	{
		return val + 6 * (val / 10);
	}

	byte bcdTodec(byte val)
	{
		return val - 6 * (val >> 4);
	}
}

Time::Time():
	lastSync(0),
	interval(SECS_PER_DAY), // sync daily
	utc(SECS_PER_HOUR) // +1h Berlin
{
	ntpServer[0] = IPAddress(192, 53, 103, 108);
	ntpServer[1] = IPAddress(81, 94, 123, 17);
	Wire.begin();
}

void Time::createNtpPacket()
{
	memset(buffer, 0, PACKET_SIZE); 

	buffer[0] = 0b11100011;   // LI, Version, Mode
	buffer[1] = 0;     // Stratum, or type of clock
	buffer[2] = 6;     // Polling Interval
	buffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	buffer[12]  = 49; 
	buffer[13]  = 0x4E;
	buffer[14]  = 49;
	buffer[15]  = 52;
}

bool Time::syncTime(const IPAddress& addr)
{	
	createNtpPacket();
	
	EthernetUDP udp;
	udp.begin(LOCAL_PORT);
	udp.beginPacket(addr, NTP_PORT); 
	udp.write(buffer, PACKET_SIZE);
	udp.endPacket();
	
	delay(1000);
	
	if (!udp.parsePacket())
		return false;

	udp.read(buffer, PACKET_SIZE);
		
	time_t high = word(buffer[40], buffer[41]);
	time_t low = word(buffer[42], buffer[43]);  
	// NTP time is seconds since 1900
	time_t ntpTime = high << 16 | low;
	// Unix time starts in 1970
	lastSync = ntpTime - SECS_PER_70YR + utc;
	DateTime dt(lastSync);
	setTime(dt);

	return true;
}

void Time::syncTime(bool force)
{
	if (!force && getTime().getUnix() - lastSync < interval)
		return;

	if (!syncTime(ntpServer[0]))
		syncTime(ntpServer[1]);
}

bool Time::isRunning() const
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(0);
	Wire.endTransmission();
	
	Wire.requestFrom(DS1307_ADDRESS, 1);
	byte sec = Wire.read();
	return !(sec >> 7);
}

void Time::setTime(DateTime& dt)
{
	byte sec = dt.getSecond();
	dt.setSecond(sec | 0x80);	// stop rtc
	writeRTC(dt);
	dt.setSecond(sec & 0x7f);	// start rtc
	writeRTC(dt);
}

bool Time::writeRTC(const DateTime& dt)
{
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(0);
	Wire.write(decTobcd(dt.getSecond()));
	Wire.write(decTobcd(dt.getMinute()));
	Wire.write(decTobcd(dt.getHour()));
	Wire.write(decTobcd(dt.getDayOfWeek()));
	Wire.write(decTobcd(dt.getDay()));
	Wire.write(decTobcd(dt.getMonth()));
	Wire.write(decTobcd(dt.getYear() - 2000));
	Wire.write(0); // default control bits 
	return !Wire.endTransmission();
}

DateTime Time::getTime() const
{
	DateTime dt;

	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(0);	
	Wire.endTransmission();
	// 7 data fields (sec, min, hr, dow, date, mth, yr)
	Wire.requestFrom(DS1307_ADDRESS, 7);
	dt.setSecond(bcdTodec(Wire.read() & 0x7f));
	dt.setMinute(bcdTodec(Wire.read()));
	dt.setHour(bcdTodec(Wire.read() & 0x3f)); // assumes 24hr 
	dt.setDayOfWeek(bcdTodec(Wire.read()));
	dt.setDay(bcdTodec(Wire.read()));
	dt.setMonth(bcdTodec(Wire.read()));
	dt.setYear(bcdTodec(Wire.read()) + 2000);
	return dt;
}

void Time::setSyncInterval(time_t intv)
{
	interval = intv;
}

void Time::setTimeServer(const IPAddress& ip)
{
	ntpServer[1] = ip;
}

void Time::setOffset(int offset)
{
	utc = offset;
}

IPAddress Time::getTimeServer() const
{
	return ntpServer[1];
}

time_t Time::getSyncInterval() const
{
	return interval;
}

int Time::getOffset() const
{
	return utc;
}
