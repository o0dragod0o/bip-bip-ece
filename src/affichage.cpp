#include "Affichage.h"

void initAffichage() {
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  setLedColor(255); 
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
}

void setLedColor(byte p) {
  analogWrite(PIN_RED, 0);
  analogWrite(PIN_GREEN, 0);
  analogWrite(PIN_BLUE, 0);

  if (p == 255) return; 

  if (p == PRIO_FAIBLE) { 
    analogWrite(PIN_RED, 0);
    analogWrite(PIN_GREEN, 255);
    analogWrite(PIN_BLUE, 0);
  } 
  else if (p == PRIO_MOYEN) { 
    analogWrite(PIN_RED, 255);
    analogWrite(PIN_GREEN, 222); 
    analogWrite(PIN_BLUE, 33);
  } 
  else if (p == PRIO_HAUTE) { 
    analogWrite(PIN_RED, 255);
    analogWrite(PIN_GREEN, 0);
    analogWrite(PIN_BLUE, 0);
  }
}

void previewSound() {
  if (alertSound == SON_CLASSIQUE) {
    digitalWrite(PIN_BUZZER, HIGH); delay(100); digitalWrite(PIN_BUZZER, LOW);
  } else if (alertSound == SON_DOUBLE) {
    digitalWrite(PIN_BUZZER, HIGH); delay(50); digitalWrite(PIN_BUZZER, LOW);
    delay(50);
    digitalWrite(PIN_BUZZER, HIGH); delay(50); digitalWrite(PIN_BUZZER, LOW);
  } else {
    digitalWrite(PIN_BUZZER, HIGH); delay(200); digitalWrite(PIN_BUZZER, LOW);
  }
}

void gererAlarmeSonore() {
  unsigned long currentMillis = millis();
  int tempo = 1000; 
  if (receivedPriority == PRIO_MOYEN) tempo = 500;
  if (receivedPriority == PRIO_HAUTE) tempo = 150; 

  if (currentMillis - buzzerTimer > (tempo / 4)) { 
    buzzerTimer = currentMillis;
    buzzerStep++;
    if (buzzerStep > 7) buzzerStep = 0; 

    bool state = false;
    if (alertSound == SON_CLASSIQUE) {
       if (buzzerStep < 4) state = true;
    }
    else if (alertSound == SON_DOUBLE) {
       if (buzzerStep == 0 || buzzerStep == 2) state = true;
    }
    else if (alertSound == SON_CONTINU) {
       if (buzzerStep % 2 == 0) state = true;
    }
    digitalWrite(PIN_BUZZER, state);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);

  if (currentMode == MODE_MENU_PRINCIPAL) {
    display.println(F("MENU PRINCIPAL:"));
    if (menuSelection == 0) display.print(F("> ")); else display.print(F("  "));
    display.println(F("ECRIRE MSG"));
    if (menuSelection == 1) display.print(F("> ")); else display.print(F("  "));
    display.println(F("REGLAGES"));
    display.setCursor(0, 50);
    display.print(myPseudo);
    display.print(F(" ["));
    if(radioSlot==0) display.print(F("A")); else display.print(F("B"));
    display.print(F("]"));
  }
  else if (currentMode == MODE_MENU_REGLAGE) {
    display.println(F("REGLAGES:"));
    if (menuSelection == 0) display.print(F(">")); else display.print(F(" "));
    display.print(F("NOM: ")); display.println(myPseudo);
    if (menuSelection == 1) display.print(F(">")); else display.print(F(" "));
    display.print(F("CANAL: ")); display.println(radioChannel);
    if (menuSelection == 2) display.print(F(">")); else display.print(F(" "));
    display.print(F("MODE: ")); 
    if(radioSlot==0) display.println(F("A")); else display.println(F("B"));
    if (menuSelection == 3) display.print(F(">")); else display.print(F(" "));
    display.print(F("SON: ")); 
    if(alertSound==SON_CLASSIQUE) display.println(F("Classique"));
    else if(alertSound==SON_DOUBLE) display.println(F("Double"));
    else display.println(F("Continu"));
    display.setCursor(0, 58);
    display.print(F("[A6] RETOUR"));
  }
  else if (currentMode == MODE_EDIT_SOUND) {
    display.println(F("CHOIX SONNERIE:"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(20, 30);
    display.setTextSize(2);
    if(alertSound==SON_CLASSIQUE) display.println(F("CLASSIQUE"));
    else if(alertSound==SON_DOUBLE) display.println(F("DOUBLE"));
    else display.println(F("CONTINU"));
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(F("Tourner=Test [A6]=OK"));
  }
  else if (currentMode == MODE_EDIT_PSEUDO) {
    display.println(F("EDIT PSEUDO:"));
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(myPseudo);
    display.setTextSize(1);
    display.setCursor(cursorPosition * 12, 35); 
    display.print(F("^"));
    display.setCursor(0, 50);
    display.print(F("Let: ")); display.println(currentLetter);
    display.print(F("[SW]SUIV [A6]SAVE"));
  }
  else if (currentMode == MODE_EDIT_CANAL) {
    display.println(F("FREQ (0-125):"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(40, 30);
    display.setTextSize(2);
    display.print(radioChannel);
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(F("[A6] SAUVEGARDER"));
  }
  else if (currentMode == MODE_EDIT_SLOT) {
    display.println(F("MODE RADIO:"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(50, 30);
    display.setTextSize(2);
    if(radioSlot==0) display.print(F("A")); else display.print(F("B"));
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(F("[A6] SAUVEGARDER"));
  }
  else if (currentMode == MODE_ALERTE_RECU) {
    display.print(F("DE: ")); display.println(receivedPseudo); 
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    display.setCursor(80, 0);
    if(receivedPriority==0) display.print(F("(V)"));
    if(receivedPriority==1) display.print(F("(J)"));
    if(receivedPriority==2) display.print(F("(R)"));
    display.setCursor(0, 15);
    display.println(sharedBuffer); 
    display.setCursor(0, 55);
    display.print(F("[A6] ACQUITTER"));
  } 
  else if (currentMode == MODE_CHOIX_PRIORITE) {
    display.println(F("URGENCE ?"));
    display.println(F("----------------"));
    display.setTextSize(2);
    display.setCursor(10, 30);
    if (selectedPriority == 0) display.println(F("FAIBLE"));
    else if (selectedPriority == 1) display.println(F("MOYEN"));
    else display.println(F("ELEVE"));
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(F("[A6] ENVOYER"));
  }
  else if (currentMode == MODE_ECRITURE) {
    display.print(F("A: ")); display.print(MAX_MESSAGE_LEN - 1 - cursorPosition);
    display.setCursor(0, 15);
    display.print(sharedBuffer); 
    if (cursorPosition < MAX_MESSAGE_LEN-1) display.print(F("_")); 
    display.drawLine(0, 42, 128, 42, SSD1306_WHITE);
    display.setCursor(0, 45);
    display.setTextSize(2);
    display.print(F("Let: "));
    display.print(currentLetter); 
  }
  display.display();
}