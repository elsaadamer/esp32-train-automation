#include <WiFi.h>
#include <ESP32Servo.h>
#include <PubSubClient.h>

const char* ssid        = "YOUR_WIFI_NAME";
const char* password    = "YOUR_WIFI_PASSWORD";
const char* mqtt_broker = "YOUR_LAPTOP_IP";   // laptop's new static IP

const int SERVO_PIN_1 = 15;
const int SERVO_PIN_2 = 13;

Servo servo1, servo2;
WiFiClient espClient;
PubSubClient mqtt(espClient);

enum BarrierState { IDLE, STEP1_MOVE_SERVO1, STEP2_WAIT, STEP3_MOVE_SERVO2 };
BarrierState state = IDLE;
unsigned long stepTime = 0;
int target1 = 90;
int target2 = 90;
bool isOpen = true;

void requestClose() {
  if (state != IDLE) return;
  target1 = 0; target2 = 0;
  state = STEP1_MOVE_SERVO1;
}

void requestOpen() {
  if (state != IDLE) return;
  target1 = 90; target2 = 90;
  state = STEP1_MOVE_SERVO1;
}

void updateBarrier() {
  switch (state) {
    case IDLE: break;
    case STEP1_MOVE_SERVO1:
      servo1.write(target1);
      stepTime = millis();
      state = STEP2_WAIT;
      break;
    case STEP2_WAIT:
      if (millis() - stepTime >= 600) state = STEP3_MOVE_SERVO2;
      break;
    case STEP3_MOVE_SERVO2:
      servo2.write(target2);
      isOpen = (target2 == 90);
      state = IDLE;
      Serial.println("Barrier move complete");
      break;
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("MQTT: "); Serial.println(msg);
  if (msg == "close" && isOpen)  requestClose();
  if (msg == "open"  && !isOpen) requestOpen();
}

void reconnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect("Barrier_ESP_1")) {
      mqtt.subscribe("barrier/1");
      Serial.println("MQTT connected");
    } else {
      Serial.print("MQTT failed rc="); Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  servo1.attach(SERVO_PIN_1);
  delay(200);
  servo2.attach(SERVO_PIN_2);
  servo1.write(90); delay(800);
  servo2.write(90); delay(800);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  mqtt.setServer(mqtt_broker, 1883);
  mqtt.setCallback(onMessage);
  reconnect();
  Serial.println("Barrier 1 ready");
}

void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();
  updateBarrier();
}