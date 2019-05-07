/* 
 *  Timeout Counters added to WiFi and MQTT connection to allow offline use of Switch 
 *  Delay time for offline switching is 10-30 seconds
 *  Flip Switch and wait for code to run in OFFLINE MODE
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi
#define CONFIG_WIFI_SSID "REDACTED"
#define CONFIG_WIFI_PASS "REDACTED"

// MQTT
#define CONFIG_MQTT_HOST "REDACTED"
#define CONFIG_MQTT_PORT 1883 // Usually 1883
#define CONFIG_MQTT_USER "REDACTED"
#define CONFIG_MQTT_PASS "REDACTED"
#define CONFIG_MQTT_CLIENT_ID "ESP_Electronics_Light" // Must be unique on the MQTT network

// MQTT Topics
#define CONFIG_MQTT_TOPIC_STATE "electronics/light/state"
#define CONFIG_MQTT_TOPIC_SET "electronics/light/switch"

#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

// PIN Definitions
#define RELAY_PIN D2
#define SWITCH_PIN D7

// Debounce
int buttonState;  
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

bool stateOn = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);

  setup_wifi();
  client.setServer(CONFIG_MQTT_HOST, CONFIG_MQTT_PORT);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(CONFIG_WIFI_SSID);

  WiFi.mode(WIFI_STA); // Disable the built-in WiFi access point.
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

  int count=0; // Timeout Counter
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if(count > 20){
      break;
    } else {
      count++;
    }
  }

  if (WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Connection Failed");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  Serial.println(message);

  if ((strcmp(message,"ON")) == 0){
    switchHandler(true);
  } else if ((strcmp(message,"OFF")) == 0){
    switchHandler(false);  
  }
  sendState();
}

void sendState() {
  if(stateOn){  
    client.publish(CONFIG_MQTT_TOPIC_STATE, CONFIG_MQTT_PAYLOAD_ON, true);
  } else {
    client.publish(CONFIG_MQTT_TOPIC_STATE, CONFIG_MQTT_PAYLOAD_OFF, true);  
  }
}

void switchHandler(boolean switchCmd){
    if(switchCmd){
        Serial.println("ON");
        digitalWrite(RELAY_PIN, HIGH);
        stateOn = true;
    } else {
        Serial.println("OFF");
        digitalWrite(RELAY_PIN, LOW);
        stateOn = false;
    }
}

void reconnect() {
  int count=0; // Timeout Counter
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_USER, CONFIG_MQTT_PASS)) {
      Serial.println("connected");
      client.subscribe(CONFIG_MQTT_TOPIC_SET);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      if(count > 2){
        break;
      }
      count++;
    }
  }
}

void checkButton(){
  int reading = digitalRead(SWITCH_PIN);
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if(stateOn){
        switchHandler(false);
      } else {
        switchHandler(true);
      }
      if (client.connected()){
        sendState();
      }
    }
  }
  lastButtonState = reading;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED){  // Try to Reconnect to WiFi if not connected
      setup_wifi();
  }
  if (WiFi.status() == WL_CONNECTED && !client.connected()) { // Try to Reconnect to MQTT Server if not connected
    reconnect();
  }
  checkButton();
  client.loop();
}
