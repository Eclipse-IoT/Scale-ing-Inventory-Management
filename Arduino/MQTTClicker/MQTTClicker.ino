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

#include <ArduinoJson.h>


// Update these with values suitable for your network.
const char* ssid = "ssid123";
const char* password = "password123";
const char* mqtt_server = "192.168.0.13";
const int port_number = 1883;
const char* mqtt_username = "";
const char* mqtt_password = "";
const bool use_username = false;
const char* device_name = "scale3";
const char* part_name = "MSK";

void ICACHE_RAM_ATTR countUp();
void ICACHE_RAM_ATTR countDown();


StaticJsonDocument<256> doc;

//Scale Config
int currentCount = 0;
int tempCount = -1;

uint8_t GPIO_Pin = D8;


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

void sendCount(){
    doc["id"] = device_name;
    doc["cnt"] = currentCount;
    doc["mass"] = '0';
    doc["pt"] = part_name;
    
    char tempString[256]; 

    serializeJson(doc, tempString);

    snprintf (msg, MSG_BUFFER_SIZE, tempString);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("cedalo/scaleOut", msg);
}

void countUp(){
  currentCount++;
}
void countDown(){
  currentCount++;
}

void setup() {
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(GPIO_Pin), countUp, RISING);
  mqttSetup();
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (tempCount != currentCount){
      tempCount = currentCount;
      sendCount();
  }
  
}
