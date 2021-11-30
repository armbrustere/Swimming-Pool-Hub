//Author: Ethan Armbruster
//Date: 11/30/21
//Rev 0.1 Swimming Pool Sensor "Hub"
#include <Arduino.h>
#include <WiFi101.h>
#include <HttpClient.h>
#include <OneWire.h>

int IOExShield_Pin_TempSensor = 2; //Temp Sensor is on Pin2 of the Extension Shield

//Temp Chip I/O
OneWire ts(IOExShield_Pin_TempSensor);

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
}

void loop() {
    float temparture = getTemp();
    Serial.println(temparture);

    delay(1000);
}


//Returns the temparture of the sensor in Degrees Celsius
float getTemp() {
    byte data[12];
    byte addr[8];

    //No Sensors found on chain, reset search
    if (!ts.search(addr)) {
        ts.reset_search();
        return -1000;
    }

    //Detects if the 8 bit CRC number from the sensor is present/valid
    //This number is located in the ROM and registers of "the sensor"?
    if(OneWire::crc8(addr,7) != addr[7])
    {
        Serial.println("CRC is not valid!");
        return -1000;
    }


}

