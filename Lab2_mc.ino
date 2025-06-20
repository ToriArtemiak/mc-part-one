#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* netID = "611VVA";
const char* netKey = "123qwerty9";

ESP8266WebServer node(80);

#define CH1 14
#define CH2 0
#define CH3 2
#define BTN 13

struct Chain {
  uint8_t port;
  bool lit;
  Chain* fwd;
  Chain* bwd;
};

Chain segA = {CH1, false, nullptr, nullptr};
Chain segB = {CH2, false, nullptr, nullptr};
Chain segC = {CH3, false, nullptr, nullptr};

Chain* current = nullptr;
bool reverse = false;
bool blinking = false;
bool activeState = false;
unsigned long marker = 0;

const uint16_t shineTime = 100;
const uint16_t pauseTime = 400;

bool lastInput = HIGH;
bool waitReset = false;
unsigned long pressMarker = 0;
unsigned long detectTime = 0;

bool remoteState = false;

void setupChain() {
  segA.bwd = &segB; segA.fwd = nullptr;
  segB.bwd = &segC; segB.fwd = &segA;
  segC.bwd = nullptr; segC.fwd = &segB;
}

void flipDirection() {
  if (blinking) return;
  reverse = !reverse;
  current = reverse ? &segC : &segA;
  blinking = true;
  marker = millis();
}

void runLeds() {
  if (!blinking) return;

  unsigned long now = millis();
  unsigned long wait = activeState ? shineTime : pauseTime;

  if (now - marker >= wait) {
    marker = now;

    digitalWrite(current->port, activeState ? HIGH : LOW);
    current->lit = activeState;

    if (!activeState) {
      current = reverse ? current->fwd : current->bwd;
      if (!current) blinking = false;
    }

    activeState = !activeState;
  }
}

void checkInput() {
  bool currentBtn = digitalRead(BTN);

  if (currentBtn != lastInput) {
    detectTime = millis();
  }

  if (millis() - detectTime > 60) {
    if (currentBtn == LOW && !waitReset) {
      pressMarker = millis();
      waitReset = true;
    } else if (currentBtn == HIGH && waitReset) {
      if (millis() - pressMarker < 400) {
        flipDirection();
      }
      waitReset = false;
    }
  }

  lastInput = currentBtn;
}

void handleStart() {
  flipDirection();
  node.send(200, "text/plain", "LOCAL LED SEQUENCE TOGGLED");
}

void handlePartner() {
  remoteState = !remoteState;
  Serial.write(remoteState ? 0x01 : 0x00);
  node.send(200, "text/plain", remoteState ? "PARTNER ON" : "PARTNER OFF");
}

void handleStatus() {
  String res = "inactive";
  if (segA.lit) res = "A";
  else if (segB.lit) res = "B";
  else if (segC.lit) res = "C";

  node.send(200, "text/plain", res);
}

void connectNet() {
  WiFi.begin(netID, netKey);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
  }
}

void initServer() {
  node.on("/local", handleStart);
  node.on("/partner", handlePartner);
  node.on("/status", handleStatus);
  node.begin();
}

void setup() {
  Serial.begin(115200);
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(CH3, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  setupChain();
  connectNet();
  initServer();
}

void loop() {
  node.handleClient();
  checkInput();
  runLeds();

  if (Serial.available()) {
    uint8_t signal = Serial.read();
    if (signal == 0x01) flipDirection();
    else if (signal == 0x00) blinking = false;
  }
}
