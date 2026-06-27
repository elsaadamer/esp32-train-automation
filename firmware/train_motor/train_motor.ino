#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid        = "YOUR_WIFI_NAME";   // removed trailing space
const char* password    = "YOUR_WIFI_PASSWORD";          // your hotspot password
const char* mqtt_broker = "YOUR_LAPTOP_IP";       // laptop static I

const int MOTOR_A1 = 33;
const int MOTOR_A2 = 32;
const int PWM_FREQ = 1000;
const int PWM_RES  = 8;

WiFiClient espClient;
PubSubClient mqtt(espClient);

void setMotor(int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    ledcWriteChannel(0, speed);
    ledcWriteChannel(1, 0);
  } else if (speed < 0) {
    ledcWriteChannel(0, 0);
    ledcWriteChannel(1, -speed);
  } else {
    ledcWriteChannel(0, 0);
    ledcWriteChannel(1, 0);
  }
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("MQTT: "); Serial.println(msg);

  if (msg == "stop")           setMotor(0);
  else if (msg == "forward")   setMotor(150);
  else if (msg == "backward")  setMotor(-150);
  else if (msg.startsWith("speed:")) {
    int spd = msg.substring(6).toInt();
    setMotor(spd);
    Serial.print("Speed: "); Serial.println(spd);
  }
}

void reconnect() {
  while (!mqtt.connected()) {
    if (mqtt.connect("Train_ESP_1")) {
      mqtt.subscribe("train/1");
      Serial.println("MQTT connected");
    } else {
      Serial.print("MQTT failed rc="); Serial.println(mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // New API in ESP32 Arduino 3.x — attach pin directly with frequency and resolution
  ledcAttach(MOTOR_A1, PWM_FREQ, PWM_RES); // channel 0 automatically
  ledcAttach(MOTOR_A2, PWM_FREQ, PWM_RES); // channel 1 automatically
  setMotor(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  mqtt.setServer(mqtt_broker, 1883);
  mqtt.setCallback(onMessage);
  reconnect();
  Serial.println("Train ready");
}

void loop() {
  if (!mqtt.connected()) reconnect();
  mqtt.loop();
}