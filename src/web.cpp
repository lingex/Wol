#include "icons.h"
#include <WiFiClient.h>
#include <WiFiServer.h>

extern WiFiServer server;
extern String macConfig;
extern String ipConfig;


extern String GetDeviceInfoString();
extern String TargetState();
extern void WolGo(String mac, String ip, int port);
extern void SaveConfig();

// Current time
unsigned long currentTime = 0;
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void WebHandle()
{
	//WiFiClient client = server.available();   // Listen for incoming clients
	WiFiClient client = server.accept();   // Listen for incoming clients
	if (client)
	{                             // If a new client connects,
		// Variable to store the HTTP request
		String header;
		currentTime = millis();
		previousTime = currentTime;
		Serial.println("New Client.");          // print a message out in the serial port
		String currentLine = "";                // make a String to hold incoming data from the client
		while (client.connected() && currentTime - previousTime <= timeoutTime)
		{ // loop while the client's connected
			currentTime = millis();
			if (client.available())
			{             // if there's bytes to read from the client,
				char c = client.read();             // read a byte, then
//				Serial.write(c);                    // print it out the serial monitor
				header += c;
				if (c == '\n')
				{                    // if the byte is a newline character
					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0)
					{
						int port = 9;
						// HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
						// and a content-type so the client knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println("Connection: close");
						client.println();
						// Display the HTML web page
						client.println("<!DOCTYPE html><html>");
						client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
						client.println("<title>WOL</title>");
						//client.println("<link rel=\"icon\" href=\"data:,\">");
						client.println("<link  rel='shortcut icon' type='image/x-icon' href=\"data:image/x-icon;base64,");
						client.println(favicon);
						client.println("\">");
						client.println("<style>");
						client.println("html {font-family: Arial; display: inline-block; text-align: center;}");
						client.println("h2 {font-size: 2.4rem;}");
						client.println("p {font-size: 2.2rem;}");
						client.println("body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}");
						client.println("input[type=\"text\"] {font-size: 16px;}");
						client.println(".button { width:120px; height:55px; padding: 15px 25px; font-size: 20px; cursor: pointer; text-align: center; text-decoration: none; outline: none; color: #fff; background-color: #2196F3; border: none; border-radius: 8px; margin: 4px 2px; }");
						client.println(".button:hover { background-color: #0354ce; }");
						client.println(".button:active { background-color: #0b16b3; }");
						//client.println("label {  display: inline-block;  width: 30px; text-align: left;  margin-right: 10px;}");
						client.println("</style>");
						//client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
						//client.println("<script src=\"https://ajax.aspnetcdn.com/ajax/jquery/jquery-3.3.1.min.js\"></script>");
						client.println("<script src=\"https://cdn.staticfile.org/jquery/1.10.2/jquery.min.js\"></script>");
						//client.println("<script src=\"/jquery.js\"></script>");
								
						// Web Page
						client.println("</head><body><h1>Wake on Lan</h1>");
						client.println("<center>");

						// target text input
						client.println("<br>");
						client.println("<br>");
						client.println("<fieldset style='width:300px; border-radius: 8px' id=\"fieldsetTarget\" onchange=\"wakeup()\">");
						client.println("<legend align='center'>Target Setting</legend>");
						client.println("<br>");
						client.println("<label for=\"macText\">Mac:</label>");
						client.println("<input type='text' name='macText' value='" + macConfig + "'/>");
						client.println("<br>");
						client.println("<br>");
						client.println("<label for=\"portText\">Port:</label>");
						client.println("<input type='text' name='portText' value='" + String(port) + "'/>");
						client.println("<br>");
						client.println("<br>");
						client.println("<label for=\"ipText\">Ip:  </label>");
						client.println("<input type='text' name='ipText' value='" + ipConfig + "'/>");
						client.println("<br>");
						client.println("<br>");
						client.println("</fieldset>");
						client.println("<br>");

						client.println("<button type=\"button\" class=\"button\" onclick=\"saveBtnAction()\">Save</button>");
						client.println("<button type=\"button\" class=\"button\" onclick=\"wakeBtnAction()\">Wake</button>");
						client.println("</p>");
						// device info print
						client.println("<div style=\"position: relative;\">");
						client.println("<div style=\"position: relative; bottom: -200px; background-color: rgb(207, 237, 248)\">");
						client.println("<label id=\"devInfo\">" + GetDeviceInfoString() + "</label>");
						client.println("</div>");
						client.println("</div>");
						
						//script start
						client.println("<script>");
						client.println("$.ajaxSetup({timeout:1000});");
						//action
						client.println("function wakeup() { ");
						client.println("var macVal = document.querySelector(\"input[name='macText']\").value;");
						client.println("var portVal = document.querySelector(\"input[name='portText']\").value;");
						client.println("var ipVal = document.querySelector(\"input[name='ipText']\").value;");
						client.println("console.log('Magic pack send:' + macVal + ', port:' + portVal + ', ip:' + ipVal);");
						client.println("{Connection: close};");
						client.println("}");
						//wake btn action
						client.println("function wakeBtnAction() {");
						client.println("var macVal = document.querySelector(\"input[name='macText']\").value;");
						client.println("var portVal = document.querySelector(\"input[name='portText']\").value;");
						client.println("var ipVal = document.querySelector(\"input[name='ipText']\").value;");
						client.println("console.log('Magic pack send, mac::' + macVal + ', port:' + portVal + ', ip:' + ipVal);");
						//client.println("console.log(\"" + GetDeviceInfoString() + "\")");

						client.println("$.get(\"/?wake=1&mac=\" + macVal + \"&port=\" + portVal + \"&ip=\" + ipVal + \"&\", function(str) ");
						client.println("{let start = str.lastIndexOf('/?resp=') + 7; let end = str.indexOf('/#', start);");
						client.println("let tstr = str.substring(start, end);");
						//client.println("console.log('str:', str); ");
						client.println("console.log('Response:', tstr);");
						client.println("document.getElementById('devInfo').innerHTML = tstr;});");
						client.println("{Connection: close};");
						client.println("}");

						//save btn action
						client.println("function saveBtnAction() {");
						client.println("var macVal = document.querySelector(\"input[name='macText']\").value;");
						client.println("var portVal = document.querySelector(\"input[name='portText']\").value;");
						client.println("var ipVal = document.querySelector(\"input[name='ipText']\").value;");
						client.println("console.log('Save para mac:' + macVal + ', port:' + portVal + ', ip:' + ipVal);");
						//client.println("console.log(\"" + GetDeviceInfoString() + "\")");

						client.println("$.get(\"/?save=1&mac=\" + macVal + \"&port=\" + portVal + \"&ip=\" + ipVal + \"&\", function(str) ");
						client.println("{let start = str.lastIndexOf('/?resp=') + 7; let end = str.indexOf('/#', start);");
						client.println("let tstr = str.substring(start, end);");
						//client.println("console.log('str:', str); ");
						client.println("console.log('Response:', tstr);");
						client.println("document.getElementById('devInfo').innerHTML = tstr;});");
						client.println("{Connection: close};");
						client.println("}");


						client.println("</script></body></html>");
						
						//GET /?wake=1&mac=11-22-33-44-55-66&port=5& HTTP/1.1
						if(header.indexOf("&mac=") >= 0)
						{
							int pos1 = header.indexOf("&mac=");
							int pos2 = header.indexOf('&', pos1 + 5);
							macConfig = header.substring(pos1 + 5, pos2);
							macConfig.replace('-', ':');
						}
						if(header.indexOf("&ip=") >= 0)
						{
							//Serial.println("debug 1->[" + header + "]");
							int pos1 = header.indexOf("&ip=");
							int pos2 = header.indexOf('&', pos1 + 4);
							ipConfig = header.substring(pos1 + 4, pos2);
							ipConfig.replace('-', '.');
							//Serial.println("debug 2->[" + ipConfig + "]");
						}
						if(header.indexOf("&port=") >= 0)
						{
							int pos1 = header.indexOf("&port=");
							int pos2 = header.indexOf('&', pos1 + 6);
							port = header.substring(pos1 + 6, pos2).toInt();
						}
						if(header.indexOf("GET /?wake=") >= 0)
						{
							Serial.println("Web request: sending magic packets->[" + macConfig + "]");
							WolGo(macConfig, ipConfig, port);
							client.println("/?resp=" + GetDeviceInfoString() + "/#");
						}
						if(header.indexOf("GET /?save=") >= 0)
						{
							Serial.println("Web request: save para->[" + macConfig + ", " + ipConfig + "]");
							SaveConfig();
							client.println("/?resp=" + GetDeviceInfoString() + "/#");
						}
//						Serial.println(header);
						// The HTTP response ends with another blank line
						client.println();
						// Break out of the while loop
						break;
					}
					else
					{ // if you got a newline, then clear currentLine
						currentLine = "";
					}
				}
				else if (c != '\r')
				{  // if you got anything else but a carriage return character,
					currentLine += c;      // add it to the end of the currentLine
				}
			}
		}
		// Clear the header variable
		header = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}