#include "Controle.h"

int lastClkState;
unsigned long lastDebounceTime = 0;

void initControle() {
  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP); 
  lastClkState = digitalRead(ENC_CLK);
}

int lireEncodeurDir() {
  int currentClkState = digitalRead(ENC_CLK);
  int direction = 0;
  
  if (currentClkState != lastClkState && currentClkState == 0) {
    if (digitalRead(ENC_DT) != currentClkState) {
      direction = 1; // Droite
    } else {
      direction = -1; // Gauche
    }
  }
  lastClkState = currentClkState;
  return direction;
}

bool lireBoutonSW() {
  if (digitalRead(ENC_SW) == LOW) {
    if (millis() - lastDebounceTime > 250) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

bool lireBoutonA6() {
  if (analogRead(BTN_SEND) < 100) {
    delay(20); // Petit debounce hardware
    if (analogRead(BTN_SEND) < 100) return true;
  }
  return false;
}