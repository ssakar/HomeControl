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

#include "ClientHelper.h"


namespace {
	const int MAX_SIZE = 32;
	char buffer[MAX_SIZE+1];
	
	inline void clearBuffer()
	{
		memset(buffer, 0, sizeof(buffer));
	}
}

ClientHelper::ClientHelper(Client* _client):
	client(_client)
{}

int ClientHelper::getRequestType()
{
	clearBuffer();
	
	if (client->readBytesUntil('/', buffer, sizeof(buffer))) {
		if (strcmp(buffer, "GET ") == 0) {
			return ClientHelper::GET;
		} else if (strcmp(buffer, "POST ") == 0) {
			return ClientHelper::POST;
		}
	}
	return ClientHelper::UNKNOWN;
}

// if uri begins with c, handle it like a GET query
const char* ClientHelper::getRequestURI(char c)
{
	clearBuffer();
	
	c = client->peek() == c ? '?' : ' ';
	
	if (client->readBytesUntil(c, buffer, sizeof(buffer))) 
		return buffer;
	
	return NULL;
}

bool ClientHelper::skipHeader()
{
	char sep[] = "\r\n\r\n";
	return client->find(sep, strlen(sep));
}

char* ClientHelper::getKey()
{
	clearBuffer();
	
	if (client->peek() == '&')
		client->read();
	
	if (client->readBytesUntil('=', buffer, sizeof(buffer)))
		return buffer;
	
	return NULL;
}

const char* ClientHelper::getValue()
{
	clearBuffer();
		
	if (client->readBytesUntil('&', buffer, sizeof(buffer)))
		return buffer;
	
	return NULL;
}

int ClientHelper::getValueInt()
{
	return client->parseInt();	
}

float ClientHelper::getValueFloat()
{
	return client->parseFloat();
}

const uint8_t* ClientHelper::getValueIP()
{
	static uint8_t ip[4] = {0};
	
	for (int i = 0; i < 4; i++)
		ip[i] = client->parseInt();
	
	return ip;
}
