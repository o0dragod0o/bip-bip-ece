/*
* PROJET BIP BIP ECE - VERSION PCB SOUDÉ (A6 PULL-UP)
* * LOGIQUE INVERSÉE :
* - Le bouton connecte A6 au GND (Masse).
* - Le code détecte l'appui quand la valeur tombe proche de 0.
*/

#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- IDENTIFICATION RADIO ---
bool radioNumber = 1; // 0 ou 1

// --- ECRAN ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- PINS ---
#define PIN_CE 7
#define PIN_CSN 8
#define PIN_BUZZER 10

// --- BOUTONS ---
#define ENC_CLK 3      
#define ENC_DT 4       
#define ENC_SW 2       
#define BTN_SEND A6    // PCB SOUDÉ SUR A6

RF24 radio(PIN_CE, PIN_CSN);
const byte addresses[][6] = {"00001", "00002"};

// --- VARIABLES ---
const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 "; 
int charsetLen = sizeof(charset) - 1;
int charIndex = 0; 
char messageBuffer[32]; 
int cursorPosition = 0; 

// Variables Gestion
int lastClkState;
unsigned long lastDebounceTime = 0;
unsigned long messageTimer = 0;
bool showingReceived = false;

// --- PROTOTYPES ---
void updateDisplayComposition();
void envoyerMessage();
void afficherMessageRecu(char* msg);
void resetComposition();
bool verifierAppuiBasA6(); // Fonction modifiée (Active Low)

void setup() {
  Serial.begin(9600);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW); 
  
  pinMode(ENC_CLK, INPUT);
  pinMode(ENC_DT, INPUT);
  pinMode(ENC_SW, INPUT_PULLUP); 

  lastClkState = digitalRead(ENC_CLK);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  display.setTextColor(SSD1306_WHITE);
  
  if (!radio.begin()) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Erreur Radio !");
    display.display();
    while (1);
  }
  radio.setPALevel(RF24_PA_MIN);

  if (radioNumber == 0) {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1, addresses[0]);
  } else {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
  }
  radio.startListening();

  resetComposition();
}

void loop() {
  // --- 1. RECEPTION ---
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    afficherMessageRecu(text);
    messageTimer = millis();
    showingReceived = true;
  }

  if (showingReceived && (millis() - messageTimer > 4000)) {
    showingReceived = false;
    updateDisplayComposition(); 
  }

  if (showingReceived) return; 

  // --- 2. ENCODEUR ---
  int currentClkState = digitalRead(ENC_CLK);
  if (currentClkState != lastClkState && currentClkState == 0) {
    if (digitalRead(ENC_DT) != currentClkState) {
      charIndex++; 
      if (charIndex >= charsetLen) charIndex = 0;
    } else {
      charIndex--; 
      if (charIndex < 0) charIndex = charsetLen - 1;
    }
    updateDisplayComposition(); 
  }
  lastClkState = currentClkState;

  // --- 3. VALIDATION LETTRE (D2) ---
  if (digitalRead(ENC_SW) == LOW) { 
    if (millis() - lastDebounceTime > 250) { 
      if (cursorPosition < 30) { 
        messageBuffer[cursorPosition] = charset[charIndex];
        cursorPosition++;
        messageBuffer[cursorPosition] = '\0'; 
      }
      updateDisplayComposition();
      lastDebounceTime = millis();
    }
  }

  // --- 4. ENVOI MESSAGE (A6 INVERSÉ / PULL UP) ---
  // On appelle la fonction corrigée
  if (verifierAppuiBasA6()) { 
      envoyerMessage(); 
      resetComposition(); 
      delay(500); 
  }
}

// --- FONCTIONS ---

// NOUVELLE LOGIQUE : On cherche le 0V (GND)
bool verifierAppuiBasA6() {
  int lecture = analogRead(BTN_SEND);
  
  // 1. Détection : Est-ce que la valeur est proche de 0 ? (Bouton enfoncé vers GND)
  // On prend < 100 pour être sûr (0V = 0, mais on laisse une marge de bruit)
  if (lecture > 100) return false; // Si > 100, c'est relâché (5V ou flottant haut)

  // 2. Filtrage (Anti-Rebond)
  // On vérifie que ça reste à 0 pendant 40ms
  for (int i = 0; i < 20; i++) {
    delay(2); 
    if (analogRead(BTN_SEND) > 100) {
      return false; // C'est remonté, donc c'était un parasite
    }
  }

  // 3. Confirmation : C'est bien un appui long à la masse
  return true;
}

void resetComposition() {
  memset(messageBuffer, 0, sizeof(messageBuffer));
  cursorPosition = 0;
  charIndex = 0; 
  updateDisplayComposition();
}

void updateDisplayComposition() {
  if (showingReceived) return; 

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Msg: ");
  display.print(cursorPosition);
  display.print("/30");

  display.setCursor(0, 15);
  display.print(messageBuffer);

  display.setCursor(0, 45);
  display.setTextSize(2);
  display.print("Let: ");
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); 
  display.print(charset[charIndex]);
  display.setTextColor(SSD1306_WHITE); 

  display.display();
}

void envoyerMessage() {
  radio.stopListening();
  
  display.clearDisplay();
  display.display(); // Ecran noir
  
  radio.write(&messageBuffer, sizeof(messageBuffer));
  
  delay(100); 
  radio.startListening();
}

void afficherMessageRecu(char* msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("RECU :");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  display.setCursor(0, 25);
  display.setTextSize(2);
  display.println(msg);
  
  display.display();
}