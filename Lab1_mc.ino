#include <stdint.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define LED1_PIN D7
#define LED2_PIN D6
#define LED3_PIN D3
#define BUTTON_PIN D5

const char* ssid = "611VVA";
const char* password = "123qwerty9";

ESP8266WebServer server(80);

uint8_t ledsOrder[3] = { LED1_PIN, LED2_PIN, LED3_PIN };
bool reverseDirection = false;

uint32_t clickInterval = 300;
uint32_t lastClickTime = 0;
uint8_t clickCount = 0;

bool buttonState = digitalRead(BUTTON_PIN) == LOW;
bool lastButtonState = HIGH;
uint32_t timeAtThisMoment = millis();

uint8_t lastActiveLed = 0;
uint32_t lastUpdateTime = 0;
uint32_t updateInterval = 300;


void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("START");
}

void setupServer() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("\nConnected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  if (buttonState && lastButtonState == HIGH) {
    if (timeAtThisMoment - lastClickTime < clickInterval) {
      clickCount++;
    } else {
      clickCount = 1;
    }
    lastClickTime = timeAtThisMoment;
  }

  if (clickCount == 2) {
    clickCount = 0;
    reverseDirection = !reverseDirection;

    if (reverseDirection) {
      Serial.println("reverseDirection ON");
    } else {
      Serial.println("reverseDirection OFF");
    }
  }

  if (timeAtThisMoment - lastClickTime > clickInterval) clickCount = 0;
  lastButtonState = buttonState;

  MainAlgorithm();
}

void MainAlgorithm() 
{
  if (millis() - lastUpdateTime >= updateInterval) return;

  digitalWrite(LedsOrder[lastActiveLed], LOW);

  if (reverseDirection) {
    if (lastActiveLed == 0) {
      lastActiveLed = 2;
    } else {
      lastActiveLed--;
    }
  } else {
    if (lastActiveLed == 2) {
      lastActiveLed = 0;
    } else {
      lastActiveLed++;
    }
  }


  digitalWrite(LedsOrder[LastActiveLed], HIGH);
  Serial.print("LED: ");
  Serial.println(LedsOrder[LastActiveLed]);

  LastUpdateTime = millis();
}

void handleRoot() {
  String html = "<html>"
                "<head>"
                "<meta charset='UTF-8'>"
                "<title>{ESP8266}</title>"

                "<style>"
                "body { text-align:center; font-family:sans-serif; background-color: #000000; margin-top: 300px; padding: 0; }"
                "h1 { color: #ffffff; font-size: 36px; margin-top: 50px; }"
                "button { padding: 15px 30px; font-size: 20px; background-color: #ffffff; border: none; color: #000000; border-radius: 20px; cursor: pointer; transition: background-color 0.3s, transform 0.3s; }"
                "button:hover { background-color: #04d7fa; transform: scale(1.1); }"
                "button:active { background-color: #0447fa; transform: scale(1.05); }"
                "</style>"

                "<script>"
                "function toggleDirection() { fetch('/toggle'); }"
                "</script>"
                "</head>"
                "<body>"
                "<h1>Хочеш змінити алгоритм?</h1>"
                "<button onclick='toggleDirection()'>Натисти!</button>"
                "</body>"
                "</html>";

  server.send(200, "text/html", html);
}

void handleToggle() {
  reverseDirection = !reverseDirection;
  if (reverseDirection) {
    Serial.println("Reverse direction ON");
  } else {
    Serial.println("Reverse direction OFF");
  }
  server.send(200, "text/plain", "OK");
}
