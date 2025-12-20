#pragma once

#include "canshu.h"

void setupPins();
void setupPwm();
void setupDisplay();
void setupWebServer();

void handleFlowPulse();
InputStatus readInputs();
void runStateMachine(const InputStatus &inputs, OutputCommand &outputs);
void applyOutputs(const OutputCommand &outputs);
void updateDisplay();

void handleGetParams();
void handleSetParams();

void setValveWithPwm(uint8_t pin, uint8_t channel, bool open);

