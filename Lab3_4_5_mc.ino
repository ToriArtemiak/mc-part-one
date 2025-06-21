#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_RESET -1
#define SENSOR_PIN 4
#define SENSOR_MODEL DHT22

const char *ssid = "611VVA";
const char *password = "123qwerty9";

const char *mqtt_server = "indigoqueen-dd9b9cf9.a03.euc1.aws.hivemq.cloud";
const int mqtt_port = 8883;
const char *mqtt_user = "Iotlabs";
const char *mqtt_pass = "Va110011";
const char *mqtt_topic = "mk_lab";

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET);
DHT dht(SENSOR_PIN, SENSOR_MODEL);

WiFiClientSecure secureClient;
PubSubClient client(secureClient);

uint32_t lastSensorRead = 0;
const uint32_t sensorInterval = 5000;

bool wifiConnected = false;
bool mqttConnected = false;
uint32_t wifiLastAttempt = 0;
uint32_t mqttLastAttempt = 0;
uint8_t mqttAttempts = 0;

const uint32_t wifiRetryInterval = 500;
const uint32_t mqttRetryInterval = 5000;
const uint8_t mqttMaxAttempts = 5;

void mqttCallback(char* topic, byte* payload, uint32_t length) {
  String message;
  for (uint32_t i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print(" ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(message, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (DISPLAY_WIDTH - w) / 2;
  int16_t y = (DISPLAY_HEIGHT - h) / 2;

  display.setCursor(x, y);
  display.println(message);
  display.display();
}

void tryConnectWiFi() {
  if (wifiConnected || (millis() - wifiLastAttempt < wifiRetryInterval)) return;

  wifiLastAttempt = millis();
  Serial.print("Connecting to WiFi");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint32_t startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    delay(100); // Коротка пауза, щоб не блокувати повністю
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\nWiFi failed");
    wifiConnected = false;
  }
}


void tryConnectMQTT() {
  if (!wifiConnected || mqttConnected || (millis() - mqttLastAttempt < mqttRetryInterval) || mqttAttempts >= mqttMaxAttempts) return;

  mqttLastAttempt = millis();
  mqttAttempts++;

  secureClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  Serial.print("Connecting to MQTT");

  String clientId = "ESP8266Client-" + String(ESP.getChipId());
  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println(" connected");
    client.subscribe(mqtt_topic);
    Serial.print("Subscribed to ");
    Serial.println(mqtt_topic);
    mqttConnected = true;
  } else {
    Serial.print(" failed, rc=");
    Serial.print(client.state());
    Serial.println(" — retrying...");
  }
}

void readAndPublishSensors() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String payload = String(temperature, 1) + "C " + String(humidity, 1) + "%";

  if (client.publish(mqtt_topic, payload.c_str())) {
    Serial.print(" ");
    Serial.println(payload);
  } else {
    Serial.println("Failed to publish");
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(payload, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (DISPLAY_WIDTH - w) / 2;
  int16_t y = (DISPLAY_HEIGHT - h) / 2;

  display.setCursor(x, y);
  display.println(payload);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(0, 5); // SDA = GPIO0 (D3), SCL = GPIO5 (D1)

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();

  dht.begin();
  tryConnectWiFi();
  tryConnectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    tryConnectWiFi();
  } else {
    wifiConnected = true;
  }

  if (!client.connected()) {
    mqttConnected = false;
    tryConnectMQTT();
  } else {
    mqttConnected = true;
  }

  client.loop();

  uint32_t now = millis();
  if (now - lastSensorRead > sensorInterval && mqttConnected) {
    lastSensorRead = now;
    readAndPublishSensors();
  }
}
