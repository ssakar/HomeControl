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

#include <ClientHelper.h>
#include <DateTime.h>
#include <Event.h>
#include <HumidSensor.h>
#include <LightSensor.h>
#include <RingBuffer.h>
#include <Schedule.h>
#include <Sensor.h>
#include <Switch.h>
#include <TempSensor.h>
#include <Time.h>
#include <Util.h>
#include <WebServer.h>
#include <SavedArray.h>

#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <IPAddress.h>
#include <RCSwitch.h>
#include <OneWire.h>
#include <DHT.h>


const int PIN_DHT11 = 7;
const int PIN_TEMP	= 6;
const int PIN_SEND = 5;
// interrupt 
const int PIN_RECV = 0;
// analog
const int PIN_LIGHT = 0;

const uint32_t MAGIC = 1410;
const uint32_t WAIT_PERIOD = 60000;
const int EVENT_DELAY = 5;
const int SEND_REPEAT = 3;
const int SERVER_PORT = 80;
const int MAX_SENSORS = 3;
const int MAX_SWITCHES = 16;
const int MAX_SCHEDULES = 32;
const int MAX_RULES = 32;
const int MAX_EVENTS = 32;

const char* URI_TIME = "time";
const char* URI_SERVER = "server";
const char* URI_SWITCH = "switch";
const char* URI_EVENT_RULES = "eventRules";
const char* URI_SCHEDULE = "schedule";
const char* URI_STATUS = "status";
const char* URI_CONTROL = "control";
const char* URI_MOBILE = "mobile";
const char* URI_FAVICON = "favicon.ico";
const char* URI_EVENT = "event";
const char* URI_SETTING = "setting";

EEMEM uint32_t magic_ee;
/* BUG 
EEMEM Switch switch_ee[MAX_SWITCHES];
EEMEM Schedule schedule_ee[MAX_SCHEDULES];
EEMEM Time time_ee;
EEMEM WebServer webServer_ee;
*/
EEMEM byte switch_ee[MAX_SWITCHES*sizeof(Switch)];
EEMEM byte schedule_ee[MAX_SCHEDULES*sizeof(Schedule)];
EEMEM byte rules_ee[MAX_RULES*sizeof(EventRule)];
EEMEM byte time_ee[sizeof(Time)];
EEMEM byte webServer_ee[sizeof(WebServer)];

SavedArray<Switch, MAX_SWITCHES> switches(&switch_ee);
SavedArray<Schedule, MAX_SCHEDULES> schedules(&schedule_ee);
SavedArray<EventRule, MAX_RULES> eventRules(&rules_ee);
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
		eventRules.load();
		timeConf.load();
		serverConf.load();
		DEBUG_PRINT("config loaded from eeprom");
	} else {
		switches.save();
		schedules.save();
		eventRules.save();
		timeConf.save();
		serverConf.save();
		eeprom_write_dword(&magic_ee, MAGIC);
		DEBUG_PRINT("config written to eeprom");
	}
	DEBUG_PRINT(int(&magic_ee));
	DEBUG_PRINT(int(&switch_ee));
	DEBUG_PRINT(int(&rules_ee));
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

	time.begin();
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
		unsigned long id = switchControl.getReceivedValue();
		for (int i = 0; i < eventRules.getSize(); i++) {
			EventRule& rule = eventRules[i];
			applyEventRules(rule, id);
		}
		logEvent(id);
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

void doSwitch(Switch& sw, bool state, bool manual = false)
{
	if (sw.isPin()) {
		pinMode(sw.getId(), OUTPUT);
		digitalWrite(sw.getId(), state);
	} else if (state) {
		for (int i = 0; i < SEND_REPEAT; i++) {
			switchControl.switchOn(sw.getGroup(), sw.getDevice());
			delay(5);
		}
	} else {
		for (int i = 0; i < SEND_REPEAT; i++) {
			switchControl.switchOff(sw.getGroup(), sw.getDevice());
			delay(5);
		}
	}
	if (!manual)
		sw.setOn(state);
	DEBUG_PRINT("switched");
}

void doManualSwitch(Switch& sw, bool state)
{
	doSwitch(sw, state, true);
}

void applyEventRules(const EventRule& rule, unsigned long id)
{
	if (!rule.isActive())
		return;

	if (rule.getEventId() != id)
		return;

	if (rule.getSwitchId() >= MAX_SWITCHES)
		return;

	Switch& sw = switches[rule.getSwitchId()];

	if (!sw.isActive())
		return;

	if (rule.toggle())
		doSwitch(sw, !sw.isOn());
	else
		doManualSwitch(sw, rule.turnOn());
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

void handleGetRequest(EthernetClient& client)
{
	ClientHelper webClient(&client);
	const char* uri = webClient.getRequestURI('c');
	DEBUG_PRINT(uri);

	if (!uri || strcmp(uri, URI_STATUS) == 0) {
		sendStatus(client);
	} else if (strcmp(uri, URI_CONTROL) == 0) {
		handleControl(client);
	} else if (strcmp(uri, URI_MOBILE) == 0) {
		sendMobile(client);
	} else if (strcmp(uri, URI_FAVICON) == 0) {
		sendError(client);
	} else if (!webClient.isAuthorized(webServer.getPassw())) {
		sendAuth(client);
	} else if (strcmp(uri, URI_SWITCH) == 0) {
		sendSwitches(client);
	} else if (strcmp(uri, URI_SCHEDULE) == 0) {
		sendSchedules(client);
	} else if (strcmp(uri, URI_EVENT) == 0) {
		sendEvents(client);
	} else if (strcmp(uri, URI_EVENT_RULES) == 0) {
		sendEventRules(client);
	} else if (strcmp(uri, URI_SETTING) == 0) {
		sendSettings(client);
	} else {
		sendError(client);
	}
}

void handlePostRequest(EthernetClient& client)
{
	ClientHelper webClient(&client);
	const char* uri = webClient.getRequestURI();
	DEBUG_PRINT(uri);

	if (!webClient.isAuthorized(webServer.getPassw())) {
		sendAuth(client);
		return;
	}
	webClient.skipHeader();

	if (strcmp(uri, URI_TIME) == 0) {
		handleTime(client);
	} else if (strcmp(uri, URI_SERVER) == 0) {
		handleServer(client);
	} else if (strcmp(uri, URI_SWITCH) == 0) {
		handleSwitches(client);
	} else if (strcmp(uri, URI_EVENT_RULES) == 0) {
		handleEventRules(client);
	} else if (strcmp(uri, URI_SCHEDULE) == 0) {
		handleSchedules(client);
	} else {
		sendError(client);
	}
}

void handleControl(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;

	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "switchon") == 0) {
			byte id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
			doManualSwitch(switches[id], true);
		} else if (strcmp(key, "switchoff") == 0) {
			byte id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
			doManualSwitch(switches[id], false);
		} else if (strcmp(key, "toggle") == 0) {
			byte id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
			Switch& sw = switches[id];
			doSwitch(sw, !sw.isOn());
		} else if (strcmp(key, "redirect") == 0) {
			redirect(client, webClient.getValue());
			return;
		} else {
			webClient.getValue(); // consume value of unknown key
		}
	}
	sendHtmlHeader(client);
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
	redirect(client, URI_SETTING);
	return;
ERROR:
	sendBadConfig(client);
}

void handleServer(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	bool dhcp = false,
		reboot = false,
		clear = false;

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
			webServer.setPassw(webClient.getValue());
			DEBUG_PRINT(webServer.getPassw());
		} else if (strcmp(key, "clear") == 0) {
			webClient.getValue();
			clear = true;
		} else if (strcmp(key, "reboot") == 0) {
			webClient.getValue();
			reboot = true;
		} else
			webClient.getValue(); // consume value of unknown key
	}
	if (clear) {
		eeprom_write_dword(&magic_ee, MAGIC + 5);
		reboot = true;
		DEBUG_PRINT("cleared eeprom");
	} else {
		webServer.setDHCP(dhcp);
		serverConf.save();
	}
	if (reboot)
		reset();
	redirect(client, URI_SETTING);
	return;
ERROR:
	sendBadConfig(client);
}

void handleEventRules(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	byte id = 0;

	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "id") == 0) {
			id = webClient.getValueInt();
			if (id >= eventRules.getSize())
				goto ERROR;
		} else if (strcmp(key, "active") == 0) {
			eventRules[id].setActive(webClient.getValueInt());
		} else if (strcmp(key, "name") == 0) {
			eventRules[id].setName(webClient.getValue());
		} else if (strcmp(key, "eventId") == 0) {
			eventRules[id].setEventId(atol(webClient.getValue()));
		} else if (strcmp(key, "switchId") == 0) {
			eventRules[id].setSwitchId(webClient.getValueInt());
		} else if (strcmp(key, "action") == 0) {
			int v = webClient.getValueInt();
			if (v == 2)
				eventRules[id].setToggle(true);
			else
				eventRules[id].setOn(v);
		} else if (strcmp(key, "clearLog") == 0) {
			if (webClient.getValueInt())
				eventLog.clear();
		} else
			webClient.getValue(); // consume value of unknown key
	}
	eventRules.save();
	redirect(client, URI_EVENT_RULES);
	return;
ERROR:
	sendBadConfig(client);
}

void handleSwitches(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	byte id = 0;
	bool pin = false;

	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "id") == 0) {
			id = webClient.getValueInt();
			if (id >= switches.getSize())
				goto ERROR;
		} else if (strcmp(key, "active") == 0) {
			switches[id].setActive(webClient.getValueInt());
		} else if (strcmp(key, "name") == 0) {
			switches[id].setName(webClient.getValue());
		} else if (strcmp(key, "group") == 0) {
			switches[id].setGroup(webClient.getValue());
		} else if (strcmp(key, "device") == 0) {
			switches[id].setDevice(webClient.getValue());
		} else if (strcmp(key, "pin") == 0) {
			webClient.getValue();
			pin = true;
		} else if (strcmp(key, "pinId") == 0) {
			switches[id].setId((byte)webClient.getValueInt());
		} else
			webClient.getValue(); // consume value of unknown key
	}
	switches[id].setPin(pin);
	switches.save();
	redirect(client, URI_SWITCH);
	return;
ERROR:
	sendBadConfig(client);
}

void handleSchedules(EthernetClient& client)
{
	ClientHelper webClient(&client);
	char* key = NULL;
	byte id = 0, swid = 0;
	bool active = false;
	Week_t w;
	w.days = 0;

	while ((key = webClient.getKey()) != NULL) {
		DEBUG_PRINT(key);
		if (strcmp(key, "id") == 0) {
			id = webClient.getValueInt();
			if (id >= schedules.getSize())
				goto ERROR;
		} else if (strcmp(key, "active") == 0) {
			active = webClient.getValueInt();
			schedules[id].setActive(active);
		} else if (strcmp(key, "name") == 0) {
			schedules[id].setName(webClient.getValue());
		} else if (strcmp(key, "switch") == 0) {
			swid = webClient.getValueInt();
			if (swid < switches.getSize())
				schedules[id].setSwitchId(swid);
		} else if (strcmp(key, "sensor") == 0) {
			byte v = webClient.getValueInt();
			if (v < MAX_SENSORS)
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
	switches[swid].setScheduled(active);
	switches.save();
	schedules.save();
	redirect(client, URI_SCHEDULE);
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

void sendHtmlHeader(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("HTTP/1.0 200 OK\r\n") << 
		F("Content-Type: text/html\r\n") << 
		F("Connection: close\r\n") << 
		F("\r\n");
}

void sendHeader(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHtmlHeader(client);
	client << F("<!DOCTYPE html><html><head><title>HomeControl</title>") <<
		F("<style type='text/css'>") <<
		F("body {color: white; background: black;}") <<
		F("a {color: white; background: black;}") <<
		F("fieldset.inline-block {display: inline-block; min-width: 300px;}") <<
		F("label {display: block; width: 100px; float: left; margin: 2px 4px 6px 4px; text-align: right;}") <<
		F("br {clear: left;}") <<
		F("</style></head>") <<
		F("<body><header><h1>HomeControl</h1><hr></header>") <<
		F("<nav><a href='status'>Status</a> | ") << 
		F("<a href='event'>Events</a> | ") <<
		F("<a href='eventRules'>Rules</a> | ") <<
		F("<a href='switch'>Switches</a> | ") <<
		F("<a href='schedule'>Schedules</a> | ") <<
		F("<a href='setting'>Settings</a><hr></nav>\n");
}

void sendFooter(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("<footer><hr>") <<
		F("<a href='mobile'>Mobile</a> | <a href='status'>Desktop</a>") <<
		F("<br>Free RAM: ") << freeRam() <<
		F("<br>Time: ") << time.getTime() <<
		F("<br>Uptime: ") << millis()/1000 <<
		F("</footer></body></html>\n");
}

void sendMobile(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHtmlHeader(client);
	client << F("<!DOCTYPE html><html><head><title>HomeControl</title>") <<
		F("<meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1'>") <<
		F("<style type='text/css'>body {color: white; background: black;}") <<
		F("a {color: white;} a.btn {display: inline-block; padding: 15px; margin: 5px;") <<
		F("border: 1px solid #303030; background-color: #909090; width: 100px;") <<
		F("height: 100px; text-align: center; text-decoration: none;") <<
		F("font-size: large; font-weight: bold;} a.on {background-color: #000066;}") <<
		F("</style></head>") <<
		F("<body><section id='main'><center>\n");

	for (int i = 0; i < switches.getSize(); i++) {
		Switch& sw = switches[i];

		if (!sw.isActive() || sw.isScheduled())
			continue;

		client << F("<a class='btn") << (sw.isOn() ? " on" : "") <<
			F("' href='control?toggle=") << i <<
			F("&redirect=mobile&'>") << sw.getName() << F("</a>\n");
	}
	client << F("</center></section>\n");
	sendFooter(client);
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

	for (int i = 0; i < switches.getSize(); i++) {
		Switch& sw = switches[i];

		if (!sw.isActive())
			continue;

		client << F("<tr><td>") <<
			i << F("</td><td>") <<
			sw.getName() << F("</td><td>") <<
			F("<a href='control?switchon=") << i << F("&redirect=status&'>On</a> | ") <<
			F("<a href='control?switchoff=") << i << F("&redirect=status&'>Off</a> | ") <<
			F("<a href='control?toggle=") << i << F("&redirect=status&'>Toggle</a>") <<
			F("</td></tr>\n");
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
		F("<label>Id: </label><input type='number' name='id' min='0' max='255' value='0'>") <<
		F("<select name='active'><option value='1' selected>Enable</option>") <<
		F("<option value='0'>Disable</option></select><br>") <<
		F("<label>Name: </label><input type='text' name='name' value='My Switch'><br>") <<
		F("<label>Group: </label><input type='text' name='group' value='11111'><br>") <<
		F("<label>Device: </label><input type='text' name='device' value='10000'><br>") <<
		F("<label>Pin: </label><input type='checkbox' name='pin'>") <<
		F(" Id: <input type='number' name='pinId' min='14' max='49'><br>") <<
		F("<label></label><input type='submit' value='Add'>") <<
		F("</fieldset></form>\n") <<

		F("<table><tr><th>Id</th><th>Name</th><th>Group</th>") <<
		F("<th>Device</th><th>Pin</th><th>State</th><th></th></tr>\n");

	for (int i = 0; i < switches.getSize(); i++) {
		Switch& sw = switches[i];

		if (!sw.isActive())
			continue;

		client << F("<tr><td>") << 
			i << F("</td><td>") <<
			sw.getName() << F("</td><td>") <<
			sw.getGroup() << F("</td><td>") <<
			sw.getDevice() << F("</td><td>") <<
			(sw.isPin() ? "Yes/" : "No/") << sw.getId() << F("</td><td>") <<
			(sw.isOn() ? "On" : "Off") << F("</td></tr>\n");
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
		F("<label>Id: </label><input type='number' name='id' min='0' max='255' value='0'>") <<
		F("<select name='active'><option value='1' selected>Enable</option>") <<
		F("<option value='0'>Disable</option></select><br>") <<
		F("<label>Name: </label><input type='text' name='name' value='My Schedule'><br>") <<
		F("<label>Time: </label><input type='datetime-local' autocomplete='on' name='time'><br>") <<
		F("<label>Duration: </label><input type='number' name='duration' min='1' max='1440' value='60'>min<br>") <<
		F("Sun <input type='checkbox' name='sun'>") <<
		F("Mon <input type='checkbox' name='mon'>") <<
		F("Tue <input type='checkbox' name='tue'>") <<
		F("Wed <input type='checkbox' name='wed'>") <<
		F("Thu <input type='checkbox' name='thu'>") <<
		F("Fri <input type='checkbox' name='fri'>") <<
		F("Sat <input type='checkbox' name='sat'>") <<
		F("All <input type='checkbox' name='all'><br>") <<
		F("<label>SwitchId: </label><input type='number' name='switch' min='0' max='255' value='255'><br>") <<
		F("<label>SensorId: </label><input type='number' name='sensor' min='0' max='255' value='255'><br>") <<
		F("<label>Threshold: </label><input type='text' name='threshold' value='100'><br>") <<
		F("<label>Action: </label><select name='on'><option value='1' selected>On</option>") <<
		F("<option value='0'>Off</option></select><br>") <<
		F("<label></label><input type='submit' value='Add'>") <<
		F("</fieldset></form>\n") <<

		F("<table><tr><th>Id</th><th>Name</th><th>Time</th>") <<
		F("<th>Duration</th><th>Days</th><th>SwitchId</th>") <<
		F("<th>SensorId</th><th>Threshold</th><th>Action</th></tr>\n");

	for (int i = 0; i < schedules.getSize(); i++) {
		Schedule& sched = schedules[i];

		if (!sched.isActive())
			continue;

		client << F("<tr><td>") << 
			i << F("</td><td>") <<
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

void sendEventRules(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'>") <<
		F("<form action='/eventRules' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>New Event Rule</legend>") <<
		F("<label>Id: </label><input type='number' name='id' min='0' max='255' value='0'>") <<
		F("<select name='active'><option value='1' selected>Enable</option>") <<
		F("<option value='0'>Disable</option></select><br>") <<
		F("<label>Name: </label><input type='text' name='name' value='My Rule'><br>") <<
		F("<label>EventId: </label><input type='text' name='eventId'><br>") <<
		F("<label>SwitchId: </label><input type='number' name='switchId' min='0' max='255' value='255'><br>") <<
		F("<label>Action: </label><select name='action'><option value='1' selected>On</option>") <<
		F("<option value='0'>Off</option><option value='2'>Toggle</option></select><br>") <<
		F("<label></label><input type='submit' value='Add'>") <<
		F("</fieldset></form>\n") <<

		F("<table><tr><th>Id</th><th>Name</th><th>EventId</th>") <<
		F("<th>SwitchId</th><th>Toggle</th><th>Action</th></tr>\n");

	for (int i = 0; i < eventRules.getSize(); i++) {
		EventRule& rule = eventRules[i];

		if (!rule.isActive())
			continue;

		client << F("<tr><td>") << 
			i << F("</td><td>") <<
			rule.getName() << F("</td><td>") <<
			rule.getEventId() << F("</td><td>") <<
			rule.getSwitchId() << F("</td><td>") <<
			(rule.toggle() ? "Yes" : "No") << F("</td><td>") <<
			(rule.turnOn() ? "On" : "Off") << F("</td></tr>\n");
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

	client << F("</table><br>\n") <<
		F("<form action='/eventRules' method='POST'>") <<
		F("<input type='hidden' name='clearLog' value='1'>") <<
		F("<input type='submit' value='Clear'></form>") <<
		F("</section>\n");

	sendFooter(client);
}

void sendSettings(EthernetClient& client)
{
	DEBUG_PRINT();
	sendHeader(client);
	client << F("<section id='main'>") <<
		F("<form action='/time' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>Time</legend>") <<
		F("<label>NTP Server: </label><input type='text' name='server' value='") << time.getTimeServer() << F("'><br>") <<
		F("<label>Sync Interval: </label><input type='number' name='interval' min='1' max='240' value='") << time.getSyncInterval()/3600 << F("'><br>") <<
		F("<label>UTC Offset: </label><input type='number' name='offset' min='-12' max='12' value='") << time.getOffset()/3600 << F("'><br>") <<
		F("<label></label><input type='submit' value='Save'>") <<
		F("</fieldset></form>") <<

		F("<form action='/server' method='POST'>") <<
		F("<fieldset class='inline-block'><legend>Server</legend>") <<
		F("<label>DHCP: </label><input type='checkbox' name='dhcp' ") << (webServer.getDHCP() ? "checked" : "") << F("><br>") <<
		F("<label>IP Address: </label><input type='text' name='ip' value='") << webServer.getIP() << F("'><br>") <<
		F("<label>Gateway: </label><input type='text' name='gw' value='") << webServer.getGW() << F("'><br>") <<
		F("<label>Subnet: </label><input type='text' name='mask' value='") << webServer.getMask() << F("'><br>") <<
		F("<label>DNS Server: </label><input type='text' name='dns' value='") << webServer.getDNS() << F("'><br>") <<
		F("<label>Password: </label><input type='password' name='host'><br>") << 
		F("<label>Clear Settings: </label><input type='checkbox' name='clear'><br>") <<
		F("<label>Reboot: </label><input type='checkbox' name='reboot'><br>") <<
		F("<label></label><input type='submit' value='Save'>") <<
		F("</fieldset></form></section>\n");

	sendFooter(client);
}

void sendBadConfig(EthernetClient& client)
{
	DEBUG_PRINT();
	sendError(client);
}

void redirect(EthernetClient& client, const char* uri)
{
	DEBUG_PRINT();
	client << F("HTTP/1.0 303 See Other\r\n") << 
		F("Location: /") << uri <<
		F("\r\n\r\n");
}

void sendAuth(EthernetClient& client)
{
	DEBUG_PRINT();
	client << F("HTTP/1.0 401 Authorization Required\r\n") <<
		F("WWW-Authenticate: Basic realm='HomeControl'\r\n") <<
		F("\r\n");
}

