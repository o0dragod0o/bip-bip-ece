/*
* PROJET BIP BIP ECE - VERSION MODULAIRE
*/

#include "Config.h"
#include "Sauvegarde.h"
#include "Affichage.h"
#include "Radio.h"

// --- VARIABLES GLOBALES (DEFINITIONS) ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RF24 radio(PIN_CE, PIN_CSN);
const byte pipes[][6] = {"00001", "00002"};

char currentLetter = 'A'; 
char sharedBuffer[MAX_MESSAGE_LEN]; 
char myPseudo[PSEUDO_LEN]; 
char receivedPseudo[PSEUDO_LEN];     

int cursorPosition = 0; 
byte currentMsgId = 0; 
byte selectedPriority = PRIO_FAIBLE; 
byte receivedPriority = PRIO_FAIBLE;

byte radioChannel; 
byte radioSlot; 
byte alertSound;

Mode currentMode = MODE_MENU_PRINCIPAL;
int menuSelection = 0; 
int lastClkState;
unsigned long lastDebounceTime = 0;
unsigned long buzzerTimer = 0;
int buzzerStep = 0;

// --- FONCTIONS LOGIQUES LOCALES ---
void lettreSuivante() {
  if (currentLetter == 'Z') currentLetter = '0';
  else if (currentLetter == '9') currentLetter = ' ';
  else if (currentLetter == ' ') currentLetter = 'A';
  else currentLetter++;
}

void lettrePrecedente() {
  if (currentLetter == 'A') currentLetter = ' ';
  else if (currentLetter == ' ') currentLetter = '9';
  else if (currentLetter == '0') currentLetter = 'Z';
  else currentLetter--;
}

bool verifierAppuiBasA6() {
  if (analogRead(BTN_SEND) < 100) {
    delay(20); 
    if (analogRead(BTN_SEND) < 100) return true;
  }
  return false;
}

void resetSystem() {
  currentMode = MODE_MENU_PRINCIPAL;
  menuSelection = 0;
  memset(sharedBuffer, 0, MAX_MESSAGE_LEN);
  setLedColor(255); 
  digitalWrite(PIN_BUZZER, LOW);
  updateDisplay();
}

void setup() {
  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP); 
  lastClkState = digitalRead(ENC_CLK);

  initAffichage();
  loadSettings();
  initRadio();
  configurerRadio();
  resetSystem(); 
}

void loop() {
  
  // 1. Radio (Vérifie si msg reçu)
  ecouterRadio();

  // 2. Gestion Entrées
  int currentClkState = digitalRead(ENC_CLK);
  bool encMoved = (currentClkState != lastClkState && currentClkState == 0);
  bool encDir = (digitalRead(ENC_DT) != currentClkState); 
  
  bool btnClicked = (digitalRead(ENC_SW) == LOW);
  if (btnClicked && millis() - lastDebounceTime < 250) btnClicked = false; 
  if (btnClicked) lastDebounceTime = millis();

  bool btnA6 = verifierAppuiBasA6();

  switch (currentMode) {
    // MENU PRINCIPAL
    case MODE_MENU_PRINCIPAL:
      if (encMoved) {
        menuSelection = !menuSelection; 
        updateDisplay();
      }
      if (btnClicked) { 
        if (menuSelection == 0) {
          currentMode = MODE_ECRITURE;
          cursorPosition = 0;
          memset(sharedBuffer, 0, MAX_MESSAGE_LEN);
        } else {
          currentMode = MODE_MENU_REGLAGE;
          menuSelection = 0;
        }
        updateDisplay();
      }
      break;

    // MENU REGLAGES
    case MODE_MENU_REGLAGE:
      if (encMoved) {
        if (encDir) {
           menuSelection++;
           if(menuSelection > 3) menuSelection = 0;
        } else {
           menuSelection--;
           if(menuSelection < 0) menuSelection = 3;
        }
        updateDisplay();
      }
      if (btnClicked) {
        if (menuSelection == 0) { // PSEUDO
           currentMode = MODE_EDIT_PSEUDO;
           cursorPosition = strlen(myPseudo);
           if(cursorPosition >= PSEUDO_LEN - 1) cursorPosition = 0;
           currentLetter = 'A';
        } else if (menuSelection == 1) { // CANAL
           currentMode = MODE_EDIT_CANAL;
        } else if (menuSelection == 2) { // SLOT
           currentMode = MODE_EDIT_SLOT;
        } else if (menuSelection == 3) { // SOUND
           currentMode = MODE_EDIT_SOUND;
        }
        updateDisplay();
      }
      if (btnA6) { 
        currentMode = MODE_MENU_PRINCIPAL;
        updateDisplay();
        delay(400);
      }
      break;

    // EDITER PSEUDO
    case MODE_EDIT_PSEUDO:
      if (encMoved) {
        if (encDir) lettreSuivante(); else lettrePrecedente();
        updateDisplay();
      }
      if (btnClicked) { 
        if (cursorPosition < PSEUDO_LEN - 1) {
          myPseudo[cursorPosition] = currentLetter;
          cursorPosition++;
          myPseudo[cursorPosition] = '\0';
        } else {
          cursorPosition = 0;
        }
        updateDisplay();
      }
      if (btnA6) { 
        saveSettingsAll();
        currentMode = MODE_MENU_REGLAGE;
        updateDisplay();
        delay(400);
      }
      break;

    // EDITER CANAL
    case MODE_EDIT_CANAL:
      if (encMoved) {
        if (encDir) { if (radioChannel < 125) radioChannel++; }
        else { if (radioChannel > 0) radioChannel--; }
        configurerRadio(); 
        updateDisplay();
      }
      if (btnClicked || btnA6) { 
        saveSettingsAll();
        currentMode = MODE_MENU_REGLAGE;
        updateDisplay();
        delay(400);
      }
      break;

    // EDITER SLOT
    case MODE_EDIT_SLOT:
      if (encMoved) {
        radioSlot = !radioSlot;
        updateDisplay();
      }
      if (btnClicked || btnA6) {
        saveSettingsAll();
        configurerRadio(); 
        currentMode = MODE_MENU_REGLAGE;
        updateDisplay();
        delay(400);
      }
      break;

    // EDITER SOUND
    case MODE_EDIT_SOUND:
      if (encMoved) {
        if (encDir) { 
          alertSound++; if(alertSound > 2) alertSound = 0; 
        } else { 
          if(alertSound == 0) alertSound = 2; else alertSound--; 
        }
        updateDisplay();
        previewSound(); 
      }
      if (btnClicked || btnA6) {
        saveSettingsAll();
        currentMode = MODE_MENU_REGLAGE;
        updateDisplay();
        delay(400);
      }
      break;

    // ECRITURE
    case MODE_ECRITURE:
      setLedColor(255); 
      if (encMoved) {
        if (encDir) lettreSuivante(); else lettrePrecedente();
        updateDisplay(); 
      }
      if (btnClicked) { 
        if (cursorPosition < MAX_MESSAGE_LEN - 1) { 
          sharedBuffer[cursorPosition] = currentLetter;
          cursorPosition++;
          sharedBuffer[cursorPosition] = '\0'; 
        }
        updateDisplay();
      }
      if (btnA6) {
         currentMode = MODE_CHOIX_PRIORITE;
         selectedPriority = PRIO_FAIBLE; 
         updateDisplay();
         delay(500);
      }
      break;

    // CHOIX PRIORITE
    case MODE_CHOIX_PRIORITE:
      setLedColor(selectedPriority);
      if (encMoved) {
        if (encDir) {
           selectedPriority++;
           if (selectedPriority > 2) selectedPriority = 0;
        } else {
           if (selectedPriority == 0) selectedPriority = 2;
           else selectedPriority--;
        }
        updateDisplay(); 
      }
      if (btnA6) {
          envoyerMessageLong();
          currentMode = MODE_MENU_PRINCIPAL; 
          setLedColor(255);
          updateDisplay();
          delay(500);
      }
      if (btnClicked) {
          currentMode = MODE_ECRITURE;
          updateDisplay();
      }
      break;

    // ALERTE RECU
    case MODE_ALERTE_RECU:
      gererAlarmeSonore();
      if (btnA6) {
         setLedColor(255); 
         digitalWrite(PIN_BUZZER, LOW);
         currentMode = MODE_MENU_PRINCIPAL;
         updateDisplay();
         delay(500);
      }
      break;
  }
  
  lastClkState = currentClkState;
}