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

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "Arduino.h"
#include "IPAddress.h"


class WebServer {
public:	
	WebServer();

	byte* getMAC();
	bool getDHCP() const;
	IPAddress getIP() const;
	IPAddress getGW() const;
	IPAddress getDNS() const;
	IPAddress getMask() const;
	const char* getPassw() const;
	
	void setDHCP(bool);
	void setIP(const IPAddress&);
	void setGW(const IPAddress&);
	void setDNS(const IPAddress&);
	void setMask(const IPAddress&);
	void setPassw(const char*);
	
	static const byte PASSW_SIZE = 10;
private:
	bool dhcp;
	IPAddress ip;
	IPAddress gw;
	IPAddress dns;
	IPAddress mask;
	byte mac[6];
	char passw[PASSW_SIZE+1];
};

#endif