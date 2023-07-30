#include <Arduino.h>
#include <WiFiServer.h>
#include <ArduinoOTA.h>
#include <OneButton.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <EEPROM.h>
#include "web.h"

#include <ESP8266Ping.h>
#include <ESP8266WiFi.h>
#include <TimeLib.h>
#include <sntp.h>
#include <TZ.h>


#define TIME_ZONE TZ_Asia_Shanghai

const String hw_ver = "1.0";
const String sw_ver = "1.0";
const String dev_name = "Wol-V1.0";
const String build_time = String(__DATE__) /*+ ", " + String(__TIME__)*/;

const char* ssid = "your ssid";
const char* password = "your passwd";

String macConfig = "00:00:00:00:00:00";
String ipConfig = "192.168.8.1";

//GPIO
#define PIN_LED 13
#define PIN_BTN 0
#define PIN_RELAY 5

#define EEPROM_SIZE_MAC 18
#define EEPROM_SIZE_IP_MIN 7
#define EEPROM_SIZE_IP_MAX 16
#define EEPROM_SIZE_MAX (EEPROM_SIZE_MAC + EEPROM_SIZE_IP_MAX + 1)

long check1s = 0;
long magicT = 0;
int connLostCnt = 0;
int runClk = 0;

WiFiUDP UDP;
WakeOnLan WOL(UDP);
WiFiServer server(80);

void ConfigWifi();
void SetupOTAConfig();
void WolGo(String mac, String ip, int port);
String TargetState();
String GetDeviceInfoString();
void GpioInit();
void SaveConfig();
void LoadConfig();


void setup() {
	Serial.begin(115200);
	Serial.println("");
	Serial.println("Wol Device");
	WOL.setRepeat(3, 100); // Optional, repeat the packet three times with 100ms between. WARNING delay() is used between send packet function.

	GpioInit();

	ConfigWifi();
	SetupOTAConfig();

	WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); // Optional  => To calculate the broadcast address, otherwise 255.255.255.255 is used (which is denied in some networks).
	pinMode(PIN_BTN, INPUT);
	server.begin();

	LoadConfig();

	configTime(TIME_ZONE, "pool.ntp.org");
	time_t now = time(nullptr);
	while (now < SECS_YR_2000)
	{
		delay(100);
		now = time(nullptr);
	}
	setTime(now);

	Serial.println("Mac address read: " + macConfig + ", ip: " + ipConfig);
	Serial.println("Device info: " + GetDeviceInfoString());
}

void loop() {
	auto ms = millis();
	if (ms - check1s > 1000)
	{
		check1s = ms;
		if (runClk++ > 4)
		{
			digitalWrite(PIN_LED, HIGH);
			runClk = 0;
		}
		ArduinoOTA.handle();
		digitalWrite(PIN_LED, LOW);

		if (!WiFi.isConnected())
		{
			if (connLostCnt++ >= 180)
			{
				ESP.restart();
			}
		}
		else
		{
			connLostCnt = 0;
		}
	}
	if (digitalRead(PIN_BTN) == 0 && ms - magicT > 3000)
	{
		magicT = ms;
		Serial.println("Btn detected: sending magic packets");
		WolGo(macConfig, "", 9);
	}
	WebHandle();
}

String TargetState()
{
	String ret = "DISABLE";

	if (!WiFi.isConnected())
	{
		return "NOCONNECT";
	}

	if (ipConfig.length() < EEPROM_SIZE_IP_MIN || ipConfig.length() > EEPROM_SIZE_IP_MAX)
	{
		return "ERR";
	}

	//IPAddress ip (192, 168, 8, 158);
	//if(Ping.ping(ip, 3))
	Serial.println("Pinging " + ipConfig);
	if(Ping.ping(ipConfig.c_str(), 3))
	{
		Serial.println("Ping ok.");
		ret = "ON";
	}
	else
	{
		Serial.println("Ping fail.");
		ret = "OFF";
	}
	return ret;
}

void WolGo(String mac, String ip, int port)
{
	const char *MACAddress = mac.c_str();

	//TODO change the port may cos crash

	digitalWrite(PIN_RELAY, HIGH);
	digitalWrite(PIN_LED, HIGH);
	WOL.sendMagicPacket(MACAddress); // Send Wake On Lan packet with the above MAC address. Default to port 9.

	delay(500);
	digitalWrite(PIN_LED, LOW);
	digitalWrite(PIN_RELAY, LOW);
}

String GetDeviceInfoString()
{
	String result = "Dev: " + dev_name;
	result += ", Hw ver: " + hw_ver;
	result += ", Sw ver: " + sw_ver;
	result += ", build: " + build_time;
	result += ", State: " + TargetState();

	unsigned long seconds = millis() / 1000;
	int days = seconds / (24 * 3600);
	seconds = seconds % (24 * 3600);
	int hours = seconds / 3600;
	seconds = seconds % 3600;
	int minutes = seconds / 60;
	seconds = seconds % 60;

	result += ", Uptime:" + String(days) + "d " + String(hours) + ":" + String(minutes) + ":" + String(seconds);

	return result;
}

void ConfigWifi()
{
	Serial.println("Wifi starting...");
	WiFi.setHostname(dev_name.c_str());
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	for (int i = 0; WiFi.status() != WL_CONNECTED && i < 20; i++)
	{
		delay(500);
		Serial.print(".");
	}
	if (WiFi.status() != WL_CONNECTED)
	{
		WiFi.mode(WIFI_AP_STA);
		WiFi.beginSmartConfig();
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(200);
		}
		WiFi.stopSmartConfig();
		WiFi.mode(WIFI_STA);
	}
	Serial.println("");
	Serial.println("Wifi connected");
}

void SetupOTAConfig()
{
	ArduinoOTA.onStart([] {
		digitalWrite(PIN_LED, HIGH);
	});
	ArduinoOTA.onProgress([](u32_t pro, u32_t total) {
		//sprintf(buf, "%d / %d", pro, total);

		//int pros = pro * 100 / total;

	});
	ArduinoOTA.onEnd([] {
		digitalWrite(PIN_LED, LOW);
	});
	ArduinoOTA.begin();
}

void GpioInit()
{
	pinMode(PIN_LED, OUTPUT);
	digitalWrite(PIN_LED, LOW);
	pinMode(PIN_RELAY, OUTPUT);
	digitalWrite(PIN_RELAY, LOW);
}

void SaveConfig()
{
	EEPROM.begin(EEPROM_SIZE_MAX);
	Serial.println("Saving para mac: " + macConfig + ", ip: " + ipConfig);

	for (size_t i = 0; i < EEPROM_SIZE_MAC; i++)
	{
		EEPROM.write(i, macConfig[i]);
	}

	int ipSize = ipConfig.length();
	Serial.println("Debug write ipSize: " + String(ipSize));
	EEPROM.write(EEPROM_SIZE_MAC, ipSize);
	for (int i = 0; i < ipSize; i++)
	{
		EEPROM.write(i + EEPROM_SIZE_MAC + 1, ipConfig[i]);
	}

	EEPROM.commit();
	EEPROM.end();
}

void LoadConfig()
{
	EEPROM.begin(EEPROM_SIZE_MAX);
	char buf[EEPROM_SIZE_MAC] = {0};
	for (size_t i = 0; i < EEPROM_SIZE_MAC; i++)
	{
		buf[i] = EEPROM.read(i);
	}
	macConfig = buf;

	int ipSize = EEPROM.read(EEPROM_SIZE_MAC);
	if (ipSize >= EEPROM_SIZE_IP_MIN && ipSize <= EEPROM_SIZE_IP_MAX)
	{
		ipConfig.clear();
		Serial.println("Debug read ipSize: " + String(ipSize));
		for (int i = 0; i < ipSize; i++)
		{
			char c = EEPROM.read(i + EEPROM_SIZE_MAC + 1);
			ipConfig += String(c);
			//Serial.println("Debug read byte: " + String(c));
		}
	}
	Serial.println("Debug ipConfig size: " + String(ipConfig.length()));
	EEPROM.end();
}
