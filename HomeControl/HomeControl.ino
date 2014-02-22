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


#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <IPAddress.h>
#include <RCSwitch.h>
#include <OneWire.h>
#include <DHT.h>

#include <Util.h>
#include <Switch.h>
#include <Sensor.h>
#include <DateTime.h>
#include <Time.h>
#include <Event.h>
#include <Schedule.h>
#include <TempSensor.h>
#include <LightSensor.h>
#include <HumidSensor.h>
#include <ClientHelper.h>
#include <WebServer.h>
#include <SavedArray.h>
#include <RingBuffer.h>


const int PIN_DHT11 = 7;
const int PIN_TEMP	= 6;
const int PIN_SEND = 5;
// interrupt 
const int PIN_RECV = 0;
// analog
const int PIN_LIGHT = 0;

const uint32_t MAGIC = 1350;
const uint32_t WAIT_PERIOD = 60000;
const int EVENT_DELAY = 5;
const int SERVER_PORT = 80;
const int MAX_SENSORS = 3;
const int MAX_SWITCHES = 16;
const int MAX_SCHEDULES = 32;
const int MAX_EVENTS = 32;


EEMEM uint32_t magic_ee;
/* BUG 
EEMEM Switch switch_ee[MAX_SWITCHES];
EEMEM Schedule schedule_ee[MAX_SCHEDULES];
EEMEM Time time_ee;
EEMEM WebServer webServer_ee;
*/
EEMEM byte switch_ee[MAX_SWITCHES*sizeof(Switch)];
EEMEM byte schedule_ee[MAX_SCHEDULES*sizeof(Schedule)];
EEMEM byte time_ee[sizeof(Time)];
EEMEM byte webServer_ee[sizeof(WebServer)];

SavedArray<Switch, MAX_SWITCHES> switches(&switch_ee);
SavedArray<Schedule, MAX_SCHEDULES> schedules(&schedule_ee);
SavedArray<Time, 1> timeConf(&time_ee);
SavedArray<WebServer, 1> serverConf(&webServer_ee);
RingBuffer<Event, MAX_EVENTS> eventLog;

Time& time = timeConf.instance();
WebServer& webServer = serverConf.instance();
RCSwitch switchControl = RCSwitch();
Sensor* sensors[MAX_SENSORS] = {0};
EthernetServer server(SERVER_PORT);

uint32_t wait;

void setup()
{
#ifdef DEBUG
	Serial.begin(9600);
#endif

	DEBUG_PRINT("starting...");
	DEBUG_PRINT(MAGIC);
	if (eeprom_read_dword(&magic_ee) == MAGIC) {
		switches.load();
		schedules.load();
		timeConf.load();
		serverConf.load();
		DEBUG_PRINT("config loaded from eeprom");
	} else {
		switches.save();
		schedules.save();
		timeConf.save();
		serverConf.save();
		eeprom_write_dword(&magic_ee, MAGIC);
		DEBUG_PRINT("config written to eeprom");
	}
	DEBUG_PRINT(int(&magic_ee));
	DEBUG_PRINT(int(&switch_ee));
	DEBUG_PRINT(int(&schedule_ee));
	DEBUG_PRINT(int(&time_ee));
	DEBUG_PRINT(int(&webServer_ee));
	
	sensors[0] = new TempSensor(PIN_TEMP);
	sensors[1] = new LightSensor(PIN_LIGHT);
	sensors[2] = new HumidSensor(PIN_DHT11);
	
	wait = millis() + WAIT_PERIOD;
	
	if (webServer.getDHCP()) {
		Ethernet.begin(webServer.getMAC());
		DEBUG_PRINT("using DHCP");
	} else {
		Ethernet.begin(webServer.getMAC(), 
			webServer.getIP(), 
			webServer.getDNS(), 
			webServer.getGW(), 
			webServer.getMask());
	}
	DEBUG_PRINT(Ethernet.localIP());

	switchControl.enableReceive(PIN_RECV);
	switchControl.enableTransmit(PIN_SEND);

	time.syncTime(true);
	server.begin();

	DEBUG_PRINT("setup finished");
}

void loop()
{
	EthernetClient client = server.available();
	
	if (client) {
		DEBUG_PRINT("client available");
		while (client.connected()) {
			if (client.available()) {
				ClientHelper webClient(&client);
				switch (webClient.getRequestType()) {
					case ClientHelper::GET:
						DEBUG_PRINT("GET request");
						handleGetRequest(client);
						break;
					case ClientHelper::POST:
						DEBUG_PRINT("POST request");
						handlePostRequest(client);
						break;
					default:
						DEBUG_PRINT("unknown request");
						sendError(client);
				}
				break;
			}
		}
		delay(1);
		client.stop();
		DEBUG_PRINT("client disconnected");
	}
	
	if (switchControl.available()) {
	
		logEvent(switchControl.getReceivedValue());
		switchControl.resetAvailable();
	}
	
	if (long(millis()-wait) >= 0) {
		
		for (int i = 0; i < schedules.getSize(); i++) {
			applySchedule(schedules[i]);
		}
		time.syncTime();
		Ethernet.maintain();
		wait += WAIT_PERIOD;
		DEBUG_PRINT("maintenance done");
		DEBUG_PRINT(freeRam());
		DEBUG_PRINT(time.getTime());
	}
}

void reset()
{
	DEBUG_PRINT("rebooting");
	delay(1000);
	asm volatile ("  jmp 0"); 
}

void logEvent(unsigned long id)
{
	time_t now = time.getTime().getUnix();
	Event ev(id);
	ev.setTime(now);
	
	Event& last = eventLog[eventLog.isEmpty() ? 0 : eventLog.getSize()-1];
	
	if (ev.getId() == last.getId() && 
			ev.getTime() - last.getTime() < EVENT_DELAY) {
		last.setTime(now);
		DEBUG_PRINT("duplicate received");
	} else { 
		eventLog.put(ev);
		DEBUG_PRINT("RF signal received");
	}
	DEBUG_PRINT(ev.getId());
}

void applySchedule(const Schedule& sched)
{
	if (!sched.isActive())
		return;

	if (sched.getSwitchId() >= MAX_SWITCHES)
		return;

	Switch& sw = switches[sched.getSwitchId()];

	if (!sw.isActive())
		return;

	DateTime curr = time.getTime();

	if (curr < sched.getTime() || !curr.isSameDay(sched.getDays())) 
		return;

	DateTime from = sched.getTime() % 86400L;
	DateTime until = (sched.getTime() + sched.getDuration()) % 86400L;
	DateTime now = time.getTime().getUnix() % 86400L;
	
	bool change = false;

	Sensor* sensor = sched.getSensorId() < MAX_SENSORS ?
		sensors[sched.getSensorId()] : NULL;

	if (sensor) {
		if (sensor->read() < sched.getThreshold()) {
			if  (sched.turnOn() == sw.isOn()) {
				change = true;
			}
		} else if (sched.turnOn() != sw.isOn()) {
			change = true;
		}
	} else if (from < now && now < until) {
		if  (sched.turnOn() != sw.isOn()) {
			change = true;
		}
	} else if (sched.turnOn() == sw.isOn()) {
		change = true;
	}

	if (change) 
		doSwitch(sw, !sw.isOn());
}

void doSwitch(Switch& sw, bool state)
{
	if (state)
		switchControl.switchOn(sw.getGroup(), sw.getDevice());
	else
		switchControl.switchOff(sw.getGroup(), sw.getDevice());
	
	sw.setOn(state);
	DEBUG_PRINT("outlet switched");
}

void handleGetRequest(EthernetClient& client)
{
	ClientHelper webClient(&client);
	const char* uri = webClient.getRequestURI('c');
	DEBUG_PRINT(uri);
	
	if (!uri || strcmp(uri, "status") == 0) {
		sendStatus(client);
	} else if (strcmp(uri, "switch") == 0) {
		sendSwitches(client);
	} else if (strcmp(uri, "schedule") == 0) {
		sendSchedules(client);
	} else if (strcmp(uri, "event") == 0) {
		sendEvents(client);
	} else if (strcmp(uri, "setting") == 0) {
		sendSettings(client);
	} else if (strcmp(uri,"control") == 0) {
		handleControl(client);
	} else {
		sendError(client);
	}
}

void handlePostRequest(EthernetClient& client)
{
	ClientHelper webClient(&client);
	const char* uri = webClient.getRequestURI();
	DEBUG_PRINT(uri);
	webClient.skipHeader();
	
	if (strcmp(uri, "time") == 0) {
		handleTime(client);
	} else if (strcmp(uri, "server") == 0) {
		handleServer(client);
	} else if (strcmp(uri, "switch") == 0) {
		handleSwitches(client);
	} else if (strcmp(uri, "schedule") == 0) {
		handleSchedules(client);
	} else {
		sendError(client);
	}
}

void handleControl(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	bool reboot = false;
	
	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "switchon") == 0) {
			byte id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
			doSwitch(switches[id], true);
		} else if (strcmp(key, "switchoff") == 0) {
			byte id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
			doSwitch(switches[id], false);
		} else if (strcmp(key, "toggle") == 0) {
			byte id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
			Switch& sw = switches[id];
			doSwitch(sw, !sw.isOn());
		} else if (strcmp(key, "reboot") == 0) {
			webClient.getValue();
			reboot = true;
		} else {
			webClient.getValue(); // consume value of unknown key
		}
	}
	if (reboot)
		reset();
	redirectStatus(client);
	return;
ERROR:
	sendBadConfig(client);
}

void handleTime(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	
	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "server") == 0) {
			IPAddress addr(webClient.getValueIP());
			if (!(addr == INADDR_NONE))
				time.setTimeServer(addr);
		} else if (strcmp(key, "interval") == 0) {
			time_t v = webClient.getValueInt();
			v *= 3600;
			if (v < 3600 || v > 864000)	// 1h-10d
				goto ERROR;
			time.setSyncInterval(v);
		} else if (strcmp(key, "offset") == 0) {
			int v = webClient.getValueInt();
			v *= 3600;
			time.setOffset(v);
		} else
			webClient.getValue(); // consume value of unknown key
	}
	timeConf.save();
	redirectStatus(client);
	return;
ERROR:
	sendBadConfig(client);
}

void handleServer(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	bool dhcp = false;
	
	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "dhcp") == 0) { // checkbox
			webClient.getValue();
			dhcp = true;
		} else if (strcmp(key, "ip") == 0) {
			IPAddress addr(webClient.getValueIP());
			if (!(addr == INADDR_NONE))
				webServer.setIP(addr);
		} else if (strcmp(key, "gw") == 0) {
			IPAddress addr(webClient.getValueIP());
			if (!(addr == INADDR_NONE))
				webServer.setGW(addr);
		} else if (strcmp(key, "mask") == 0) {
			IPAddress addr(webClient.getValueIP());
			if (!(addr == INADDR_NONE))
				webServer.setMask(addr);
		} else if (strcmp(key, "dns") == 0) {
			IPAddress addr(webClient.getValueIP());
			if (!(addr == INADDR_NONE))
				webServer.setDNS(addr);
		} else if (strcmp(key, "host") == 0) { 
			const char* pass = webClient.getValue();// actually password
			if (strlen(pass) > 6)
				webServer.setPassw(pass);
		} else
			webClient.getValue(); // consume value of unknown key
	}
	webServer.setDHCP(dhcp);
	serverConf.save();
	redirectStatus(client);
	return;
ERROR:
	sendBadConfig(client);
}

void handleSwitches(EthernetClient& client)
{	
	ClientHelper webClient(&client);
	char* key = NULL;
	byte id = 0;
	
	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "id") == 0) {
			id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
		} else if (strcmp(key, "active") == 0) {
			bool v = webClient.getValueInt();
			switches[id].setActive(v);
		} else if (strcmp(key, "name") == 0) {
			switches[id].setName(webClient.getValue());
		} else if (strcmp(key, "group") == 0) {
			switches[id].setGroup(webClient.getValue());
		} else if (strcmp(key, "device") == 0) {
			switches[id].setDevice(webClient.getValue());
		} else
			webClient.getValue(); // consume value of unknown key
	}
	switches.save();
	redirectStatus(client);
	return;
ERROR:
	sendBadConfig(client);
}

void handleSchedules(EthernetClient& client)
{	
	ClientHelper webClient(&client);
	char* key = NULL;
	byte id = 0;
	Week_t w;
	w.days = 0;
	
	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "id") == 0) {
			id = webClient.getValueInt();
			if (id >= schedules.getSize())
				goto ERROR;
		} else if (strcmp(key, "active") == 0) {
			schedules[id].setActive(webClient.getValueInt());
		} else if (strcmp(key, "name") == 0) {
			schedules[id].setName(webClient.getValue());
		} else if (strcmp(key, "switch") == 0) {
			byte v = webClient.getValueInt();
			if (id < switches.getSize())
				schedules[id].setSwitchId(v);
		} else if (strcmp(key, "sensor") == 0) {
			byte v = webClient.getValueInt();
			if (id < MAX_SENSORS)
				schedules[id].setSensorId(v);
		} else if (strcmp(key, "threshold") == 0) {
			schedules[id].setThreshold(webClient.getValueFloat());
		} else if (strcmp(key, "on") == 0) {
			schedules[id].setOn(webClient.getValueInt());
		} else if (strcmp(key, "time") == 0) {
			int year = client.parseInt(); // 2014-02-15T17%3A20
			client.find("-", 1);
			byte month = client.parseInt();
			client.find("-", 1);
			byte day = client.parseInt();
			byte hour = client.parseInt();
			client.find("%3A", 3); // :
			byte min = client.parseInt();
			DateTime dt(0,min,hour,0,day,month,year);
			schedules[id].setTime(dt.getUnix());
			DEBUG_PRINT(dt);
		} else if (strcmp(key, "duration") == 0) {
			time_t v = webClient.getValueInt();
			v *= 60;
			schedules[id].setDuration(v);
		} else if (strcmp(key, "sun") == 0) {
			webClient.getValue();
			w.week.sun = 1;
		} else if (strcmp(key, "mon") == 0) {
			webClient.getValue();
			w.week.mon = 1;
		} else if (strcmp(key, "tue") == 0) {
			webClient.getValue();
			w.week.tue = 1;
		} else if (strcmp(key, "wed") == 0) {
			webClient.getValue();
			w.week.wed = 1;
		} else if (strcmp(key, "thu") == 0) {
			webClient.getValue();
			w.week.thu = 1;
		} else if (strcmp(key, "fri") == 0) {
			webClient.getValue();
			w.week.fri = 1;
		} else if (strcmp(key, "sat") == 0) {
			webClient.getValue();
			w.week.sat = 1;
		} else if (strcmp(key, "all") == 0) {
			webClient.getValue();
			w.week.all = 1;
		} else
			webClient.getValue(); // consume value of unknown key
	}
	if (w.days != 0)
		schedules[id].setDays(w);
	schedules.save();
	redirectStatus(client);
	return;
ERROR:
	sendBadConfig(client);
}

void sendError(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("HTTP/1.0 400 Bad Request\r\n") << 
		F("Content-Type: text/html\r\n") << 
		F("Connection: close\r\n") << 
		F("\r\n") << 
		F("<!DOCTYPE html><html><head><title></title></head>") <<
		F("<body><h1>400 Bad Request</h1>") <<
		F("</body></html>\n");

}

void sendHeader(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("HTTP/1.0 200 OK\r\n") << 
		F("Content-Type: text/html\r\n") << 
		F("Connection: close\r\n") << 
		F("\r\n") <<
		F("<!DOCTYPE html><html><head><title>HomeControl</title>") <<
		F("<style type='text/css'>") <<
		F("body {color: white; background: black;}") <<
		F("a {color: white; background: black;}") <<
		F("fieldset.inline-block {display: inline-block;}") <<
		F("</style></head>") <<
		F("<body><header><h1>HomeControl</h1><hr></header>") <<
		F("<nav><a href='status'>Status</a> | ") << 
		F("<a href='event'>Events</a> | ") <<
		F("<a href='switch'>Switches</a> | ") <<
		F("<a href='schedule'>Schedules</a> | ") <<
		F("<a href='setting'>Settings</a><hr></nav>\n");
}

void sendFooter(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("<footer><hr>") <<
		F("Free RAM: ") << freeRam() <<
		F("<br>Time: ") << time.getTime() <<
		F("<br>Uptime: ") << millis() <<
		F("</footer></body></html>\n");
}

void sendStatus(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'><table>") <<
		F("<tr><th>Id</th><th>Name</th><th>Value</th></tr>");
	
	for (int i = 0; i < MAX_SENSORS; i++) {
		client << F("<tr><td>") << 
			i << F("</td><td>") <<
			sensors[i]->getName() << F("</td><td>") <<
			sensors[i]->read() << F("</td></tr>\n"); 
	}

	client << F("</table></section>\n");
	sendFooter(client);
}

void sendSwitches(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'>") <<
		F("<form action='/switch' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>New Switch</legend>") <<
		F("Id: <input type='number' name='id' min='0' max='255' value='0'>") <<
		F("<select name='active'><option value='1' selected>Enable</option>") <<
		F("<option value='0'>Disable</option></select><br>") <<
		F("Name: <input type='text' name='name' value='My Switch'><br>") <<
		F("Group: <input type='text' name='group' value='11111'><br>") <<
		F("Device: <input type='text' name='device' value='10000'><br>") <<
		F("<input type='submit' value='Add'>") <<
		F("</fieldset></form>\n") <<

		F("<table><tr><th>Id</th><th>Active</th><th>Name</th><th>Group</th>") <<
		F("<th>Device</th><th>State</th><th></th></tr>\n");
	
	for (int i = 0; i < switches.getSize(); i++) {
		Switch& sw = switches[i];
		client << F("<tr><td>") << 
			i << F("</td><td>") <<
			sw.isActive() << F("</td><td>") <<
			sw.getName() << F("</td><td>") <<
			sw.getGroup() << F("</td><td>") <<
			sw.getDevice() << F("</td><td>") <<
			(sw.isOn() ? "On" : "Off") << F("</td><td>") <<
			F("<a href='control?toggle=") << i << F("'>toggle</a></td></tr>\n");
	}
	client << F("</table></section>\n");
	sendFooter(client);
}

void sendSchedules(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'>") <<
		F("<form action='/schedule' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>New Schedule</legend>") <<
		F("Id: <input type='number' name='id' min='0' max='255' value='0'>") <<
		F("<select name='active'><option value='1' selected>Enable</option>") <<
		F("<option value='0'>Disable</option></select><br>") <<
		F("Name: <input type='text' name='name' value='My Schedule'><br>") <<
		F("Time: <input type='datetime-local' autocomplete='on' name='time'><br>") <<
		F("Duration: <input type='number' name='duration' min='1' max='1440' value='60'>min<br>") <<
		F("Sun <input type='checkbox' name='sun'>") <<
		F("Mon <input type='checkbox' name='mon'>") <<
		F("Tue <input type='checkbox' name='tue'>") <<
		F("Wed <input type='checkbox' name='wed'>") <<
		F("Thu <input type='checkbox' name='thu'>") <<
		F("Fri <input type='checkbox' name='fri'>") <<
		F("Sat <input type='checkbox' name='sat'>") <<
		F("All <input type='checkbox' name='all'><br>") <<
		F("SwitchId: <input type='number' name='switch' min='0' max='255' value='255'><br>") <<
		F("SensorId: <input type='number' name='sensor' min='0' max='255' value='255'><br>") <<
		F("Threshold: <input type='text' name='threshold' value='100'><br>") <<
		F("Action: <select name='on'><option value='1' selected>On</option>") <<
		F("<option value='0'>Off</option></select><br>") <<
		F("<input type='submit' value='Add'>") <<
		F("</fieldset></form>\n") <<

		F("<table><tr><th>Id</th><th>Active</th><th>Name</th><th>Time</th>") <<
		F("<th>Duration</th><th>Days</th><th>SwitchId</th>") <<
		F("<th>SensorId</th><th>Threshold</th><th>Action</th></tr>\n");

	for (int i = 0; i < schedules.getSize(); i++) {
		Schedule& sched = schedules[i];
		client << F("<tr><td>") << 
			i << F("</td><td>") <<
			sched.isActive() << F("</td><td>") <<
			sched.getName() << F("</td><td>") <<
			DateTime(sched.getTime()) << F("</td><td>") <<
			sched.getDuration()/60 << F("</td><td>") <<
			sched.getDays().days << F("</td><td>") <<
			sched.getSwitchId() << F("</td><td>") <<
			sched.getSensorId() << F("</td><td>") <<
			sched.getThreshold() << F("</td><td>") <<
			(sched.turnOn() ? "On" : "Off") << F("</td></tr>\n");
	}
	client << F("</table></section>\n");
	sendFooter(client);
}

void sendEvents(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'><table><tr><th>Id</th><th>Time</th></tr>\n");
	
	for (int i = 0; i < eventLog.getSize(); i++) {
		Event& ev = eventLog[i];
		client << F("<tr><td>") <<
			ev.getId() << F("</td><td>") <<
			DateTime(ev.getTime()) << F("</td></tr>\n");
	}

	client << F("</table></section>\n");
	sendFooter(client);
}

void sendSettings(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'>") <<
		F("<form action='/time' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>Time</legend>") <<
		F("NTP Server: <input type='text' name='server' value='") << time.getTimeServer() << F("'><br>") <<
		F("Sync Interval: <input type='number' name='interval' min='1' max='240' value='") << time.getSyncInterval()/3600 << F("'><br>") <<
		F("UTC Offset: <input type='number' name='offset' min='-12' max='12' value='") << time.getOffset()/3600 << F("'><br>") <<
		F("<input type='submit' value='Save'>") <<
		F("</fieldset></form>") <<
		
		F("<form action='/server' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>Server</legend>") <<
		F("DHCP: <input type='checkbox' name='dhcp' ") << (webServer.getDHCP() ? "checked" : "") << F("><br>") <<
		F("IP Address: <input type='text' name='ip' value='") << webServer.getIP() << F("'><br>") <<
		F("Gateway: <input type='text' name='gw' value='") << webServer.getGW() << F("'><br>") <<
		F("Subnet: <input type='text' name='mask' value='") << webServer.getMask() << F("'><br>") <<
		F("DNS Server: <input type='text' name='dns' value='") << webServer.getDNS() << F("'><br>") <<
		F("Password: <input type='password' name='host' size='10'><br>") << 
		F("<input type='submit' value='Save'>") <<
		F("</fieldset></form></section>\n");
	
	sendFooter(client);
}

void sendBadConfig(EthernetClient& client)
{
	DEBUG_PRINT();
	sendError(client);
}

void redirectStatus(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("HTTP/1.0 303 See Other\r\n") << 
		F("Location: /status\r\n") << 
		F("\r\n");
}

