#include "Affichage.h"

void initAffichage() {
  pinMode(RedPin, OUTPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);
  setLedColor(255); 
  pinMode(BuzzerPin, OUTPUT);
  digitalWrite(BuzzerPin, LOW); 

  // initialisation de l'écran
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
}

void setLedColor(byte p) {
  analogWrite(RedPin, 0); analogWrite(GreenPin, 0); analogWrite(BluePin, 0);
  if (p == 255) return; 
  if (p == LowPrio)analogWrite(GreenPin, 255);
  else if (p == AveragePrio){
    analogWrite(RedPin, 255);
    analogWrite(GreenPin, 222);
    analogWrite(BluePin, 33);
    //jaune
  }else if (p == HighPrio)analogWrite(RedPin, 255);
}
void previewSound() {
  digitalWrite(BuzzerPin, HIGH); 
  delay(alertSound == ClassicSound ? 100 : 50); // delay depend de l'alarme
  digitalWrite(BuzzerPin, LOW);
}

void handleAlarm() {
  unsigned long currentMillis = millis();
  //tempo en fonction de la priorité
  int tempo = 1000; 
  if (receivedPriority == AveragePrio) tempo = 500;
  if (receivedPriority == HighPrio) tempo = 150; 

  if (currentMillis - buzzerTimer > (tempo / 4)) { 
    buzzerTimer = currentMillis;
    buzzerStep++;
    if (buzzerStep > 7) buzzerStep = 0; 
    bool state = false;
    if (alertSound == ClassicSound && buzzerStep < 4) state = true;
    else if (alertSound == DoubleSound && (buzzerStep == 0 || buzzerStep == 2)) state = true;
    else if (alertSound == EndlessSound && (buzzerStep % 2 == 0)) state = true;
    digitalWrite(BuzzerPin, state);
  }
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);

  //MENU PRINCIPAL
  if (currentMode == MainMenu) {
    display.println(F("MENU PRINCIPAL:"));
    display.print(selectionMenu == 0 ? F("> ") : F("  "));
    display.println(F("ECRIRE MSG"));
    display.print(selectionMenu == 1 ? F("> ") : F("  "));
    display.println(F("REGLAGES"));
    display.setCursor(0, 50); display.print(myPseudo); display.print(F(" ["));
    display.print(radioSlot==0?F("A"): F("B"));
    display.print(F("]"));
  }
  //MENU REGLAGES
  else if (currentMode == SettingsMenu) {
    display.println(F("REGLAGES:"));
    display.print(selectionMenu == 0 ? F(">") : F(" "));
    display.print(F("NOM: "));
    display.println(myPseudo);
    display.print(selectionMenu == 1 ? F(">") : F(" "));
    display.print(F("CANAL: "));
    display.println(radioChannel);
    display.print(selectionMenu == 2 ? F(">") : F(" "));
    display.print(F("MODE: "));
    display.println(radioSlot==0?F("A"):F("B"));
    display.print(selectionMenu == 3 ? F(">") : F(" "));
    display.print(F("SON: "));
    display.println(alertSound);
    display.setCursor(0, 58);
    display.print(F("[A6] RETOUR"));
  }
  //SOUS-MENUS
  else if (currentMode == SoundEditMode) {
    display.println(F("CHOIX SONNERIE:"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(20, 30);
    display.setTextSize(2);
    if(alertSound==ClassicSound) display.println(F("CLASSIQUE"));
    else if(alertSound==DoubleSound) display.println(F("DOUBLE"));
    else display.println(F("CONTINU"));
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(F("Tourner=Test [A6]=OK"));
  }
  else if (currentMode == PseudoMode) {
    display.println(F("EDIT PSEUDO:"));
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(myPseudo);
    display.setTextSize(1);
    display.setCursor(cursorPosition * 12, 35);
    display.print(F("^"));
    display.setCursor(0, 50); display.print(F("Let: "));
    display.println(currentLetter);
    display.print(F("[SW]SUIV [A6]SAVE"));
  }
  else if (currentMode == CanalEditMode) {
    display.println(F("FREQ (0-125):"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(40, 30);
    display.setTextSize(2);
    display.print(radioChannel);
    display.setTextSize(1);
    display.setCursor(0, 55);
    display.print(F("[A6] SAUVEGARDER"));
  }
  else if (currentMode == SlotEditMode) {
    display.println(F("MODE RADIO:"));
    display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
    display.setCursor(50, 30); display.setTextSize(2);
    if(radioSlot==0) display.print(F("A"));
    else {
      display.print(F("B"));
      display.setTextSize(1);
    }
    display.setCursor(0, 55);
    display.print(F("[A6] SAUVEGARDER"));
  }
  else if (currentMode == AlarmMode) {
      display.print(F("DE: "));
      display.println(receivedPseudo); 
      display.setCursor(0, 15);
      display.println(sharedBuffer); 
      display.setCursor(0, 55);
      display.print(F("[A6] ACQUITTER"));
  } 
  else if (currentMode == PriorityMode) {
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

  //MODE ECRITURE
  else if (currentMode == WritingMode) {
    //En-tête fixe
    display.print(F("Msg: "));
    display.print(cursorPosition);
    display.print(F("/"));
    display.println(MaxMessageLen - 1);
    
    //Zone de texte (Scrolling)
    int maxVisible = 63;
    int startIndex = 0;
    if (cursorPosition > maxVisible) startIndex = cursorPosition - maxVisible;
    
    display.setCursor(0, 12); 
    if (startIndex > 0) display.print(F(".."));
    display.print(&sharedBuffer[startIndex]);
    if (cursorPosition < MaxMessageLen-1) display.print(F("_")); 
    
    //Pied de page
    // On remonte la ligne à Y=45 pour laisser de la place en dessous
    display.drawLine(0, 45, 128, 45, SSD1306_WHITE);
    
    // Affichage de "Choix:"
    display.setCursor(0, 52);
    display.setTextSize(1);
    display.print(F("Choix: ")); 
    
    // Affichage de la LETTRE en GROS
    display.setCursor(42, 48);
    display.setTextSize(2); 
    display.print(currentLetter); 
  }
  
  display.display();
}
