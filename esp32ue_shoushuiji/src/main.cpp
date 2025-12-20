#include <Arduino.h>
#include <WiFi.h>
#include "canshu.h"
#include "Lyd.h"

namespace {
constexpr char kApSsid[] = "esp";
constexpr char kApPassword[] = "19711128";
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  setupPins();
  setupPwm();
  setupDisplay();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(kApSsid, kApPassword);

  setupWebServer();

  attachInterrupt(digitalPinToInterrupt(PIN_LIULIANG), handleFlowPulse, RISING);

  state_message = "售水机控制";
  data_message = "系统启动完成";
  updateDisplay();
}

void loop() {
  server.handleClient();

  InputStatus inputs = readInputs();
  OutputCommand outputs{};
  runStateMachine(inputs, outputs);
  applyOutputs(outputs);
  updateDisplay();

  delay(50);
}

