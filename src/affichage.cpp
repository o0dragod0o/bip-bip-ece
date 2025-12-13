#include "Affichage.h"

void initAffichage() {
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  setLedColor(255); 
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
}

void setLedColor(byte p) {
  analogWrite(PIN_RED, 0); analogWrite(PIN_GREEN, 0); analogWrite(PIN_BLUE, 0);
  if (p == 255) return; 
  if (p == PRIO_FAIBLE) { analogWrite(PIN_GREEN, 255); } 
  else if (p == PRIO_MOYEN) { analogWrite(PIN_RED, 255); analogWrite(PIN_GREEN, 222); analogWrite(PIN_BLUE, 33); } 
  else if (p == PRIO_HAUTE) { analogWrite(PIN_RED, 255); }
}

void previewSound() {
  digitalWrite(PIN_BUZZER, HIGH); 
  delay(alertSound == SON_CLASSIQUE ? 100 : 50); 
  digitalWrite(PIN_BUZZER, LOW);
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
    if (alertSound == SON_CLASSIQUE && buzzerStep < 4) state = true;
    else if (alertSound == SON_DOUBLE && (buzzerStep == 0 || buzzerStep == 2)) state = true;
    else if (alertSound == SON_CONTINU && (buzzerStep % 2 == 0)) state = true;
    digitalWrite(PIN_BUZZER, state);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);

  // --- MENU PRINCIPAL ---
  if (currentMode == MODE_MENU_PRINCIPAL) {
    display.println(F("MENU PRINCIPAL:"));
    display.print(menuSelection == 0 ? F("> ") : F("  ")); display.println(F("ECRIRE MSG"));
    display.print(menuSelection == 1 ? F("> ") : F("  ")); display.println(F("REGLAGES"));
    display.setCursor(0, 50); display.print(myPseudo); display.print(F(" [")); display.print(radioSlot==0?F("A"):F("B")); display.print(F("]"));
  }
  // --- MENU REGLAGES ---
  else if (currentMode == MODE_MENU_REGLAGE) {
    display.println(F("REGLAGES:"));
    display.print(menuSelection == 0 ? F(">") : F(" ")); display.print(F("NOM: ")); display.println(myPseudo);
    display.print(menuSelection == 1 ? F(">") : F(" ")); display.print(F("CANAL: ")); display.println(radioChannel);
    display.print(menuSelection == 2 ? F(">") : F(" ")); display.print(F("MODE: ")); display.println(radioSlot==0?F("A"):F("B"));
    display.print(menuSelection == 3 ? F(">") : F(" ")); display.print(F("SON: ")); display.println(alertSound);
    display.setCursor(0, 58); display.print(F("[A6] RETOUR"));
  }
  // --- SOUS-MENUS ---
  else if (currentMode == MODE_EDIT_SOUND) {
    display.println(F("CHOIX SONNERIE:"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(20, 30);
    display.setTextSize(2);
    if(alertSound==SON_CLASSIQUE) display.println(F("CLASSIQUE"));
    else if(alertSound==SON_DOUBLE) display.println(F("DOUBLE"));
    else display.println(F("CONTINU"));
    display.setTextSize(1);
    display.setCursor(0, 55); display.print(F("Tourner=Test [A6]=OK"));
  }
  else if (currentMode == MODE_EDIT_PSEUDO) {
    display.println(F("EDIT PSEUDO:"));
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    display.setCursor(0, 20); display.setTextSize(2); display.print(myPseudo); display.setTextSize(1);
    display.setCursor(cursorPosition * 12, 35); display.print(F("^"));
    display.setCursor(0, 50); display.print(F("Let: ")); display.println(currentLetter);
    display.print(F("[SW]SUIV [A6]SAVE"));
  }
  else if (currentMode == MODE_EDIT_CANAL) {
    display.println(F("FREQ (0-125):"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(40, 30); display.setTextSize(2); display.print(radioChannel); display.setTextSize(1);
    display.setCursor(0, 55); display.print(F("[A6] SAUVEGARDER"));
  }
  else if (currentMode == MODE_EDIT_SLOT) {
    display.println(F("MODE RADIO:"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(50, 30); display.setTextSize(2); if(radioSlot==0) display.print(F("A")); else display.print(F("B")); display.setTextSize(1);
    display.setCursor(0, 55); display.print(F("[A6] SAUVEGARDER"));
  }
  else if (currentMode == MODE_ALERTE_RECU) {
      display.print(F("DE: ")); display.println(receivedPseudo); 
      display.setCursor(0, 15); display.println(sharedBuffer); 
      display.setCursor(0, 55); display.print(F("[A6] ACQUITTER"));
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
    display.setCursor(0, 55); display.print(F("[A6] ENVOYER"));
  }

  // --- MODE ECRITURE ---
  else if (currentMode == MODE_ECRITURE) {
    // 1. En-tête fixe
    display.print(F("Msg: "));
    display.print(cursorPosition);
    display.print(F("/"));
    display.println(MAX_MESSAGE_LEN - 1);
    
    // 2. Zone de texte (Scrolling)
    int maxVisible = 63; 
    int startIndex = 0;
    if (cursorPosition > maxVisible) startIndex = cursorPosition - maxVisible;
    
    display.setCursor(0, 12); 
    if (startIndex > 0) display.print(F(".."));
    display.print(&sharedBuffer[startIndex]);
    if (cursorPosition < MAX_MESSAGE_LEN-1) display.print(F("_")); 
    
    // 3. Pied de page (CORRIGÉ)
    // On remonte la ligne à Y=45 pour laisser de la place en dessous
    display.drawLine(0, 45, 128, 45, SSD1306_WHITE);
    
    // Affichage du label "Choix:" en petit
    display.setCursor(0, 52);
    display.setTextSize(1);
    display.print(F("Choix: ")); 
    
    // Affichage de la LETTRE en GROS juste à côté
    // On se place à X=40 (après "Choix: ") et Y=48 (pour que ça tienne en hauteur)
    display.setCursor(42, 48);
    display.setTextSize(2); 
    display.print(currentLetter); 
    
    // On dessine un petit cadre autour de la lettre pour faire joli (optionnel)
    // display.drawRect(40, 46, 16, 18, SSD1306_WHITE);
  }
  
  display.display();
}