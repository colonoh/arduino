#include <ArduinoMqttClient.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <WiFi101.h>

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
const String ssid = {SECRET_SSID};
const String pass = {SECRET_PASS};
int status = WL_IDLE_STATUS;
WiFiClient client;

MqttClient mqttClient(client);
const String broker = {"raspberrypi"};
int port = 1883;
const String topic = {"home/temperature"};

int digitalPin = 1;  // pin for digital sensor
OneWire oneWire(digitalPin);
DallasTemperature sensors(&oneWire);
int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address


void setup() {
  // attempt to connect to WiFi network:
  WiFi.setPins(8,7,4,2);  // Configure pins for Adafruit ATWINC1500 Feather
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(1000);
  }
  mqttClient.connect(broker.c_str(), port);
  sensors.begin();

  numberOfDevices = sensors.getDeviceCount();
  
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");
}

void loop() {
  sensors.requestTemperatures();
    for(int i=0;i<numberOfDevices; i++) {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      // Output the device ID
      Serial.print("Temperature for device ");  
      Serial.print(i,DEC);
      Serial.print(": "); 

      // Print the data
      float tempC = sensors.getTempC(tempDeviceAddress);
      float tempF = DallasTemperature::toFahrenheit(tempC);
      Serial.print(tempF);
      Serial.println("degF");

      mqttClient.poll();
      mqttClient.beginMessage(topic + "/" + i);
      mqttClient.print(tempF);
      mqttClient.endMessage();
    } 	
  }
  delay(1000);
}
