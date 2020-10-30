/*******************************************************************************
 * Copyright (c) 2020 Gregory Ivo
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 * 
 * Contributors:
 *    Gregory Ivo
 *******************************************************************************/

//MQTT Libraries
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//Scale Libraries
#include "HX711.h"

#include <ArduinoJson.h>


// Update these with values suitable for your network.
const char* ssid = "ssid123";
const char* password = "password123";
const char* mqtt_server = "192.168.0.13";
const int port_number = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";
const bool use_username = false;
const char* device_name = "scale1";
const char* part_name = "TP";

//Scale Pinout
#define DOUT  5
#define CLK  16
#define BTN  15

StaticJsonDocument<256> doc;

//Scale Config
HX711 scale;
float calibration_factor = -23.15; //-23.15 for grams woth 4 50GK loadcells
int weightOfOneUnit = 100;
int currentCount = 0;
int precision = 20;
int pushVal = 0;     // variable for reading the pin status

//Network/MQTT condif
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (use_username){
      if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("cedalo/scaleControl");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    }else{
          if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("cedalo/scaleControl");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
      
    }

  }
}

void mqttSetup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  setup_wifi();
  client.setServer(mqtt_server, port_number);
  client.setCallback(callback);
}


void scaleSetup() {
  //scaleSetup
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  //button setup
  pinMode(BTN, INPUT);    // declare pushbutton as input
}

void setup() {
  Serial.begin(9600);
  mqttSetup();
  digitalWrite(BUILTIN_LED, HIGH);
  scaleSetup();
  digitalWrite(BUILTIN_LED, LOW);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int tempMass = scale.get_units(precision);
  int tempCount = (tempMass / weightOfOneUnit);

  if (tempCount < 0) {
    tempCount = 0;
  }

  if (currentCount != tempCount) {
    currentCount = tempCount;
    Serial.print("Count: ");
    Serial.print(currentCount);
    Serial.print(" Mass: ");
    Serial.print(tempMass, 1);
    Serial.print("g");
    Serial.println();

    doc["id"] = device_name;
    doc["cnt"] = currentCount;
    doc["mass"] = tempMass;
    doc["pt"] = part_name;
    
    char tempString[256]; 

    serializeJson(doc, tempString);

    snprintf (msg, MSG_BUFFER_SIZE, tempString);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("cedalo/scaleOut", msg);
  }

    pushVal = digitalRead(BTN);  // read input value
  if (pushVal == HIGH) {
      Serial.print("Setting Current Weight as 1 Unit, please wait");
      Serial.println();
      do {
        weightOfOneUnit = (scale.get_units(precision) * 0.95); //need to use a more precise loadcell in the future
      }while(weightOfOneUnit < 0);
      Serial.print("one unit is: ");
      Serial.print(weightOfOneUnit);
      Serial.print("g");
      Serial.println();
  }
}
