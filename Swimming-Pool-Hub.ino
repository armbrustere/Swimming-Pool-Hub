//Author: Ethan Armbruster
//Date: 11/30/21
//Rev 0.1 Swimming Pool Sensor "Hub"

//Libraries for WiFi Connectivity
#include <WiFiNINA.h>
#include "arduino_secrets.h"
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

//Temp Sensor Library
#include <OneWire.h>

//PH Sensor Library
#include <DFRobot_PH.h>
#include <EEPROM.h>

//Data for WIFI Network Connection
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

//Creates WiFiClient object
WiFiClient client;

//Temp Sensor is on Pin13 of the Extension Shield
int IOExShield_Pin_TempSensor = 12;

//Server Connected To
// char serverName[] = "www.ptsv2.com";
// char pageName[] = "/t/sensor/post";
// int serverPort = 80;
char serverName[] = "18.210.225.101";
char pageName[] = "/sensor_post";
int serverPort = 80;
//Time Variables for keeping track of Time Interval
unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

//Set Size of postBuffer to ensure little garbage is sent
char bodyBuff[420];

//Sets timer for sending new data
#define delayTime 30000 

//Temp Chip I/O, tells OneWire what sensor to connect to
OneWire ts(IOExShield_Pin_TempSensor);
StaticJsonDocument<400> doc;


void setup() {
    Serial.begin(9600);
    attemptConnection();
}

void loop() {
   // getTemp();
    catJSON(getTemp());
    //catJSON();
    delay(delayTime);
}


//Returns the temparture of the sensor in Degrees Celsius and Farenheit
float getTemp() {
    byte data[12];
    byte addr[8];

    //No Sensors found on chain, reset search
    if (!ts.search(addr)) {
        Serial.println("No Sensors Found!");
        ts.reset_search();
        return -1000;
    }

    //Detects if the 8 bit CRC number from the sensor is present/valid
    //This number is located in the ROM and registers of "the sensor"?
    if (OneWire::crc8(addr, 7) != addr[7]) {
        Serial.println("CRC is not valid!");
        return -1000;
    }

    if (addr[0] != 0x10 && addr[0] != 0x28) {
        Serial.println("Device is not recognized");
        return -1000;
    }

    ts.reset();
    ts.select(addr);
    ts.write(0x44, 1); //Starts the conversion with parasite power on at the end.

    byte present = ts.reset();
    ts.select(addr);
    ts.write(0xBE); //Read ScratchPad

    for (int i = 0; i < 9; i++) {
        data[i] = ts.read();
    }
    ts.reset_search();

    byte MSB = data[1];
    byte LSB = data[0];

    float temperatureRead = ((MSB << 8) | LSB); //Uses two's compliment
    float celsius = temperatureRead / 16;
    float fahrenheit = celsius * 1.8 + 32.0;

    return celsius;
}

void attemptConnection() {
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("Wifi Shield not present");
        //Discontinues the connection process
        while (true);
    }

    //attempt to Connect to network
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection:
        delay(10000);
    }

    Serial.println("Connected to wifi");

    printWiFiStatus();

    Serial.println("\nStarting connection to server...");
}

void printWiFiStatus() {

    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

byte postJSON(char *domainBuffer, int domainPort, char *pageName, char *thisData) {
    int inChar;
    //Creates Buffer for Post Header
    char outBuff[64];
    int stringLength;

    Serial.print(F("connecting...."));

    if (client.connect(domainBuffer, domainPort) == 1) {
        Serial.print(F("connected!!"));

        //Sends Header
        sprintf(outBuff, "POST %s HTTP/1.1", pageName);
        client.println(outBuff);
        sprintf(outBuff, "Host: %s", domainBuffer);
        client.println(outBuff);
        client.println(F("Connection: close\r\nContent-Type: application/json"));
        client.println("User-Agent: Arduino Uno WiFi Rev2");
        stringLength = strlen(thisData);
        sprintf(outBuff, "Content-Length: %u\r\n", stringLength);
        client.println(outBuff);


        // send the body (variables)
        client.print(thisData);
    } else {
        Serial.println(F("Failed to Send"));
        return 0;
    }

    int connectLoop = 0;

    while (client.connected()) {
        while (client.available()) {
            inChar = client.read();
            Serial.write(inChar);
            connectLoop = 0;
        }
        delay(1);
        connectLoop++;

        //Closes the connection if it exceeds the time limit
        if (connectLoop > 10000) {
            Serial.println();
            Serial.println(F("Timeout"));
            client.stop();
        }
    }
    //Closes the connection after sending the data
    Serial.println();
    Serial.println(F("disconnecting."));
    client.stop();
    return 1;

}

void catJSON(float tempReading)
{
   float fahrenheit = tempReading * 1.8 + 32.0;
   doc["Pool Sensor Data"] = 1;
   JsonArray sensorInfo = doc.createNestedArray("Temperature");
   JsonObject sensorData = sensorInfo.createNestedObject();

   sensorData["Celsius"] = tempReading;
   sensorData["Fahrenheit"] = fahrenheit;
   serializeJsonPretty(doc, bodyBuff);
   doc.garbageCollect();

   if(!postJSON(serverName,serverPort,pageName,bodyBuff)) Serial.print(F("Fail "));
   else Serial.print(F("Pass "));
}
