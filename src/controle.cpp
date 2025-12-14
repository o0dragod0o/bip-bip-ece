#include "Controle.h"

int lastClkState;
unsigned long lastDebounceTime = 0;

void initControl() {
  pinMode(EncCLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(EncSW, INPUT_PULLUP); 
  lastClkState = digitalRead(EncCLK);
}

int readEncDir() {
  int currentClkState = digitalRead(EncCLK);
  int direction = 0;
  
  if (currentClkState != lastClkState && currentClkState == 0) {
    if (digitalRead(ENC_DT) != currentClkState) {
      direction = 1; // CW
    } else {
      direction = -1; // CCW
    }
  }
  lastClkState = currentClkState;
  return direction;
}

bool readSWBtn() {
  if (digitalRead(EncSW) == LOW) {
    //eviter rebonds
    if (millis() - lastDebounceTime > 250) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

bool readSendBtn() {
  if (analogRead(SendBtn) < 100) {
    delay(20); //eviter rebonds
    if (analogRead(SendBtn) < 100) return true;
  }
  return false;
}
