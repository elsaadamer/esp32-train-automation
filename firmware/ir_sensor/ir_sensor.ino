#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid        = "YOUR_WIFI_NAME";          // your laptop hotspot name
const char* password    = "YOUR_WIFI_PASSWORD";         // your laptop hotspot password
const char* mqtt_broker = "YOUR_LAPTOP_IP";     // your laptop's hotspot IP from Step 2

const int IR_PIN_1 = 34; // before barrier — triggers CLOSE
const int IR_PIN_2 = 35; // after barrier  — triggers OPEN

WiFiClient espClient;
PubSubClient mqtt(espClient);

// These run instantly on pin change — no polling delay
void IRAM_ATTR ir1_triggered() {
  // train approaching — tell barrier to close
  // can't call mqtt.publish() directly from interrupt
  // so we use a flag
  static volatile bool flag = true;
  flag = true;
}

// simpler: just use flags checked in loop
volatile bool close_barrier = false;
volatile bool open_barrier  = false;

void IRAM_ATTR isr1() { close_barrier = true; }
void IRAM_ATTR isr2() { open_barrier  = true; }

void setup() {
  Serial.begin(115200);
  pinMode(IR_PIN_1, INPUT);
  pinMode(IR_PIN_2, INPUT);

  // Interrupts fire on FALLING edge (sensor goes LOW when train detected)
  attachInterrupt(digitalPinToInterrupt(IR_PIN_1), isr1, FALLING);
  attachInterrupt(digitalPinToInterrupt(IR_PIN_2), isr2, FALLING);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  mqtt.setServer(mqtt_broker, 1883);
  while (!mqtt.connected()) {
    mqtt.connect("IR_ESP32");
    delay(500);
  }
  Serial.println("MQTT connected");
}

void loop() {
  mqtt.loop(); // keep connection alive

  if (close_barrier) {
    close_barrier = false;
    mqtt.publish("barrier/1", "close");
    Serial.println("Published: close");
  }

  if (open_barrier) {
    open_barrier = false;
    mqtt.publish("barrier/1", "open");
    Serial.println("Published: open");
  }
}