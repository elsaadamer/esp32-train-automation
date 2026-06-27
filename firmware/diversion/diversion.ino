#include <WiFi.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>

const char* ssid        = "YOUR_WIFI_NAME";   // removed trailing space
const char* password    = "YOUR_WIFI_PASSWORD";          // your hotspot password
const char* mqtt_broker = "YOUR_LAPTOP_IP";       // laptop static IP     

const int SERVO_PIN     = 15;
const int STRAIGHT_ANGLE = 65;
const int RIGHT_ANGLE    = 90;

// Change this per diversion — "diversion/1" or "diversion/2"
const char* MQTT_TOPIC  = "diversion/1";
const char* CLIENT_ID   = "Diversion_ESP_1";

int current_state = 0; // 0 = straight, 1 = right

Servo diversionServo;
WiFiClient espClient;
PubSubClient mqtt(espClient);

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("MQTT received: "); Serial.println(msg);

  if (msg == "straight" && current_state == 1) {
    diversionServo.write(STRAIGHT_ANGLE);
    current_state = 0;
    Serial.println("Diversion → STRAIGHT (90 deg)");
  }
  if (msg == "right" && current_state == 0) {
    diversionServo.write(RIGHT_ANGLE);
    current_state = 1;
    Serial.println("Diversion → RIGHT (50 deg)");
  }
}

void reconnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect(CLIENT_ID)) {
      mqtt.subscribe(MQTT_TOPIC);
      Serial.println("MQTT connected — subscribed to " + String(MQTT_TOPIC));
    } else {
      Serial.print("MQTT failed rc="); Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  diversionServo.attach(SERVO_PIN);
  diversionServo.write(STRAIGHT_ANGLE);
  delay(1000);

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
}