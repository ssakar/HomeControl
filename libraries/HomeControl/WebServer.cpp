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

#include "WebServer.h" 


namespace {

	void fixPadding(char* base, int size)
	{
		char* str = base + size - 6;

		if (strcmp(str, "%3D%3D") == 0) {
			str[0] = '=';
			str[1] = '=';
			str[2] = '\0';
		} else if (strcmp(str + 3, "%3D") == 0) {
			str[3] = '=';
			str[4] = '\0';
		}
	}
}


WebServer::WebServer():
	dhcp(true)
{
	// TODO teach arduino c++11
	mac[0] = 0x00;	
	mac[1] = 0x08;	
	mac[2] = 0xDC;	
	mac[3] = 0x00;	
	mac[4] = 0x00;	
	mac[5] = 0xAA;	
	
	// admin:admin
	strncpy(passw, "YWRtaW46YWRtaW4=", PASSW_SIZE);
}

byte* WebServer::getMAC() 
{
	return mac;	
}

bool WebServer::getDHCP() const
{
	return dhcp;
}

IPAddress WebServer::getIP() const
{
	return ip;
}

IPAddress WebServer::getGW() const
{
	return gw;
}

IPAddress WebServer::getDNS() const
{
	return dns;
}

IPAddress WebServer::getMask() const
{
	return mask;
}

char* WebServer::getPassw()
{
	return passw;
}

void WebServer::setDHCP(bool _dhcp)
{
	dhcp = _dhcp;
}

void WebServer::setIP(const IPAddress& _ip)
{
	ip = _ip;
}

void WebServer::setGW(const IPAddress& _gw)
{
	gw = _gw;
}

void WebServer::setDNS(const IPAddress& _dns)
{
	dns = _dns;
}

void WebServer::setMask(const IPAddress& _mask)
{
	mask = _mask;
}

bool WebServer::setPassw(const char* _passw)
{
	if (strlen(_passw) < MIN_PASSW_SIZE)
		return false;

	strncpy(passw, _passw, MAX_PASSW_SIZE);
	fixPadding(passw, strlen(passw));

	return true;
}

