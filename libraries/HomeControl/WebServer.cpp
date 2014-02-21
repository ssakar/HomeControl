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
	
	strncpy(passw, "0000", PASSW_SIZE);
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

const char* WebServer::getPassw() const
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

void WebServer::setPassw(const char* _passw)
{
	strncpy(passw, _passw, PASSW_SIZE);
}
