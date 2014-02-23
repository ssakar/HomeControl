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

#ifndef CLIENT_HELPER_H
#define CLIENT_HELPER_H

#include "Client.h"


class ClientHelper {
	Client* client;
	
public:
	ClientHelper(Client*);
	
	int getRequestType(); 
	const char* getRequestURI(char = '-');
	bool skipHeader();
	char* getKey();
	const char* getValue();
	int getValueInt();
	float getValueFloat();
	const uint8_t* getValueIP();

	bool isAuthorized(char*);

	static const int GET = 1;
	static const int POST = 2;
	static const int UNKNOWN = 3;
};

#endif
