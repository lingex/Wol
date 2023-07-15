#include <Arduino.h>
#include <WiFiServer.h>
#include <ArduinoOTA.h>
#include <OneButton.h>
//#include <WiFi.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <EEPROM.h>
#include "web.h"

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

//GPIO
#define PIN_LED 13
#define PIN_BTN 0
#define PIN_RELAY 5

#define EEPROM_SIZE 18

//char buf[32] = {0};
long check1s = 0;
long magicT = 0;
int connLostCnt = 0;
int runClk = 0;

WiFiUDP UDP;
WakeOnLan WOL(UDP);
WiFiServer server(80);


void ConfigWifi();
void setupOTAConfig();
void WolGo(String mac, String ip, int port);
String TargetState();
String GetDeviceInfoString();
void GpioInit();
String GetMacConfig();


void setup() {
	Serial.begin(115200);
	Serial.println("");
	Serial.println("Wol Device");
	WOL.setRepeat(3, 100); // Optional, repeat the packet three times with 100ms between. WARNING delay() is used between send packet function.

	GpioInit();

	ConfigWifi();
	setupOTAConfig();

	WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); // Optional  => To calculate the broadcast address, otherwise 255.255.255.255 is used (which is denied in some networks).
	pinMode(PIN_BTN, INPUT);
	server.begin();
	EEPROM.begin(EEPROM_SIZE);
	char buf[EEPROM_SIZE] ;
	for (size_t i = 0; i < EEPROM_SIZE; i++)
	{
		buf[i] = EEPROM.read(i);
	}
	EEPROM.end();
	macConfig = buf;
	//macConfig =  EEPROM.readString(0);

	configTime(TIME_ZONE, "pool.ntp.org");
	time_t now = time(nullptr);
	while (now < SECS_YR_2000)
	{
		delay(100);
		now = time(nullptr);
	}
	setTime(now);

	Serial.println("Mac address: " + macConfig);
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
	String ret = "TODO";

	return ret;
}

void WolGo(String mac, String ip, int port)
{
	//const char *MACAddress = "00:D8:61:79:28:11";
	const char *MACAddress = mac.c_str();

	//TODO setting port may cos crash

	digitalWrite(PIN_RELAY, HIGH);
	digitalWrite(PIN_LED, HIGH);
	WOL.sendMagicPacket(MACAddress); // Send Wake On Lan packet with the above MAC address. Default to port 9.

	if (mac != macConfig)
	{
		EEPROM.begin(EEPROM_SIZE);
		Serial.println("Saving new mac: " + mac);
		macConfig = mac;
		//EEPROM.writeString(0, mac);
		for (size_t i = 0; i < EEPROM_SIZE; i++)
		{
			EEPROM.write(i, mac[i]);
		}
		EEPROM.commit();
		EEPROM.end();
	}
	delay(400);
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

void setupOTAConfig()
{
	ArduinoOTA.onStart([] {

	});
	ArduinoOTA.onProgress([](u32_t pro, u32_t total) {
		//sprintf(buf, "%d / %d", pro, total);

		//int pros = pro * 100 / total;

	});
	ArduinoOTA.onEnd([] {

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

String GetMacConfig()
{
	return macConfig;
}
