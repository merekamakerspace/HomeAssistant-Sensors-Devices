#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "REDACTED"
#define WLAN_PASS       "REDACTED"

/************************* MQTT Setup *********************************/

#define AIO_SERVER      "REDACTED"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "REDACTED"
#define AIO_KEY         "REDACTED"
/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
// location/type/name
Adafruit_MQTT_Publish mqt = Adafruit_MQTT_Publish(&mqtt, "openarea/sensor/airquality");
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, "electronics/sensor/temp");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, "electronics/sensor/humidity");

/*************************** Sketch Code ************************************/

#define MQT135 A0
#define ALARM D5
#define THRESHOLD 150

#define DHTPIN D2     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  pinMode(MQT135, INPUT);
  pinMode(ALARM, OUTPUT);
  delay(10);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Initialize Sensor.
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  /* MQT Sensor */
  
  int sensorVal = analogRead(MQT135);
  Serial.print(F("\nSending air quality val "));
  Serial.print(sensorVal);
  Serial.print("...");
  if (sensorVal > THRESHOLD){
    digitalWrite(ALARM , HIGH);
  }else{digitalWrite(ALARM, LOW);}
  if (! mqt.publish(sensorVal)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  /* DHT SENSOR */
  
  sensors_event_t event;
  dht.temperature().getEvent(&event);
    
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
      if (! temp.publish(event.temperature)) {
      Serial.println(F("Failed"));
      } else {
        Serial.println(F("OK!"));
      }
  }
  
  dht.humidity().getEvent(&event);
  
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
      if (! humidity.publish(event.relative_humidity)) {
      Serial.println(F("Failed"));
     } else {
        Serial.println(F("OK!"));
      }
  }
  
  delay(1000);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
