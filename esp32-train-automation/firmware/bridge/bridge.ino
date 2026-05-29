#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid        = "YOUR_WIFI_NAME";   // removed trailing space
const char* password    = "YOUR_WIFI_PASSWORD";          // your hotspot password
const char* mqtt_broker = "YOUR_LAPTOP_IP";       // laptop static I

// Bridge motor pins — unchanged from original
const int DIR_PIN   = 13;
const int STEP_PIN  = 12;
const int BRIDGE_STEPS = 3000;

// Non-blocking stepper state machine
enum BridgeState { IDLE, MOVING };
BridgeState bridgeState = IDLE;
int stepsRemaining      = 0;
int stepDir             = HIGH;
unsigned long lastStep  = 0;
const int STEP_US       = 500; // same speed as original
int bridge_position     = 0;   // 0 = down, 1 = up

WiFiClient espClient;
PubSubClient mqtt(espClient);

void requestUp() {
  if (bridgeState != IDLE || bridge_position == 1) return;
  stepDir        = HIGH;
  stepsRemaining = BRIDGE_STEPS;
  bridgeState    = MOVING;
  digitalWrite(DIR_PIN, HIGH);
  delayMicroseconds(5);
  Serial.println("Bridge moving UP");
}

void requestDown() {
  if (bridgeState != IDLE || bridge_position == 0) return;
  stepDir        = LOW;
  stepsRemaining = BRIDGE_STEPS;
  bridgeState    = MOVING;
  digitalWrite(DIR_PIN, LOW);
  delayMicroseconds(5);
  Serial.println("Bridge moving DOWN");
}

// Runs every loop — steps motor one pulse at a time without blocking
void updateBridge() {
  if (bridgeState == IDLE) return;
  if (stepsRemaining <= 0) {
    bridgeState     = IDLE;
    bridge_position = (stepDir == HIGH) ? 1 : 0;
    Serial.println("Bridge move complete");
    return;
  }
  if (micros() - lastStep >= STEP_US) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);
    stepsRemaining--;
    lastStep = micros();
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("MQTT [bridge/1]: "); Serial.println(msg);

  if (msg == "up")   requestUp();
  if (msg == "down") requestDown();
}

void reconnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect("Bridge_ESP")) {
      mqtt.subscribe("bridge/1");
      Serial.println("MQTT connected — subscribed to bridge/1");
    } else {
      Serial.print("MQTT failed rc="); Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(DIR_PIN,  OUTPUT);
  pinMode(STEP_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  mqtt.setServer(mqtt_broker, 1883);
  mqtt.setCallback(onMessage);
  reconnect();
  Serial.println("Ready — waiting for commands");
}

void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();
  updateBridge(); // non-blocking step
}