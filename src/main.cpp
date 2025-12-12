#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h> // Nécessaire pour EF15 (sauvegarde)

// ---------------- HARDWARE PINS -----------------
// Définis selon le tableau de connexions [cite: 168]
#define PIN_ENCODER_A 3
#define PIN_ENCODER_B 4
#define PIN_ENCODER_SW 2
#define PIN_SEND_BUTTON A6 // Attention: Entrée analogique seulement!
#define PIN_LED_R 5
#define PIN_LED_G 6
#define PIN_LED_B 9
#define PIN_BUZZER 10      // Attention au conflit SPI 
#define PIN_NRF_CE 7
#define PIN_NRF_CSN 8

// ---------------- PARAMÈTRES & CONSTANTES ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define EEPROM_ADDR 0     // Adresse de début de sauvegarde

// Liste des caractères disponibles (EF2: Français inclus) [cite: 72]
// Note: L'affichage des accents dépend de la police CP437
const char charMap[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,!?'-_@#&()éèàç";
const int charMapLen = sizeof(charMap) - 1;

// ---------------- STRUCTURES DE DONNÉES ----------------

// Structure pour sauvegarder les réglages en EEPROM (EF15) [cite: 95]
struct Config {
  char pseudo[16];
  uint8_t canal;     // 0-125
  uint8_t melody;    // 0-2 (EF14)
  uint8_t magic;     // Pour vérifier si l'EEPROM est initialisée
};

Config settings;

// Structures pour le protocole Radio (EF4, EF5) [cite: 75, 76]
struct HeaderPacket {
  uint8_t type;         // 0 = Header
  uint8_t totalPackets;
  uint8_t msgId;        // ID unique pour éviter les doublons
  char pseudo[16];      // EF5
  uint8_t priority;     // EF3
};

struct DataPacket {
  uint8_t type;         // 1 = Data
  uint8_t seqNum;       // Numéro du paquet (1, 2, 3...)
  uint8_t msgId;        // Lien avec le Header
  char data[28];        // Payload (Max 32 octets total pour NRF24) 
};

// ---------------- VARIABLES GLOBALES ----------------
RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);
const byte pipeAddress[6] = "BIP25"; // Adresse fixe, on change la fréquence (canal)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variables d'état
String inputMessage = "";
volatile int encoderCount = 0;
int lastEncoderCount = 0;
unsigned long lastButtonPress = 0;

// États du Menu
enum State { MAIN_MENU, WRITE_MSG, SETTINGS, READ_MSG, ALERT };
State currentState = MAIN_MENU;
int menuIndex = 0;

// Réception
String receivedMsgBuffer = "";
String receivedPseudo = "";
uint8_t receivedPriority = 0;
unsigned long alertStartTime = 0;

// ---------------- GESTION ENCODEUR & BOUTONS ----------------
void encoderISR() {
  // Lecture simple de l'encodeur
  if (digitalRead(PIN_ENCODER_A) == digitalRead(PIN_ENCODER_B)) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

int getEncoderDelta() {
  int delta = encoderCount - lastEncoderCount;
  lastEncoderCount = encoderCount;
  return delta;
}

// EF9: Bouton "Envoyer" sur A6 (Analogique) 
bool isSendButtonPressed() {
  // A6 est analogique. Avec Pull-up 10k, non appuyé ~= 1023, appuyé ~= 0.
  // Seuil à 500 pour être sûr.
  if (analogRead(PIN_SEND_BUTTON) < 500) {
    if (millis() - lastButtonPress > 300) { // Debounce
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

bool isEncoderButtonPressed() {
  if (digitalRead(PIN_ENCODER_SW) == LOW) {
    if (millis() - lastButtonPress > 300) {
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

// ---------------- EEPROM (EF15) ----------------
void loadConfiguration() {
  EEPROM.get(EEPROM_ADDR, settings);
  // Si la mémoire est vierge ou corrompue, mettre les défauts
  if (settings.magic != 0x42) {
    strcpy(settings.pseudo, "USER");
    settings.canal = 76;
    settings.melody = 0;
    settings.magic = 0x42;
    EEPROM.put(EEPROM_ADDR, settings);
  }
}

void saveConfiguration() {
  EEPROM.put(EEPROM_ADDR, settings);
}

// ---------------- AUDIO & LED (EF6, EF7, EF14) ----------------
void playTone(int priority, int melodyType) {
  // Désactive le SPI buzzer si nécessaire, mais ici on utilise tone() ou analogWrite
  // EF7: Couleur selon priorité
  if (priority == 0) { // High
    analogWrite(PIN_LED_R, 255); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, 0);
  } else if (priority == 1) { // Medium
    analogWrite(PIN_LED_R, 255); analogWrite(PIN_LED_G, 128); analogWrite(PIN_LED_B, 0);
  } else { // Low
    analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 255); analogWrite(PIN_LED_B, 0);
  }

  // EF14: Choix sonorité
  // Ceci est une implémentation bloquante simple. Pour du non-bloquant, utiliser millis().
  if (melodyType == 0) {
    tone(PIN_BUZZER, 1000, 200); delay(250); tone(PIN_BUZZER, 2000, 200);
  } else if (melodyType == 1) {
    tone(PIN_BUZZER, 500, 100); delay(150); tone(PIN_BUZZER, 500, 100);
  } else {
    // Mode silencieux ou juste LED
  }
}

void stopAlert() { // EF8
  noTone(PIN_BUZZER);
  analogWrite(PIN_LED_R, 0); analogWrite(PIN_LED_G, 0); analogWrite(PIN_LED_B, 0);
}

// ---------------- RADIO (EF4, EF5, EF12) ----------------
void sendRadioMessage() {
  radio.stopListening();
  
  // Générer un ID de message aléatoire
  uint8_t msgId = random(0, 255);
  int len = inputMessage.length();
  int totalPackets = (len / 28) + ((len % 28) ? 1 : 0);

  // 1. Envoyer Header
  HeaderPacket header;
  header.type = 0;
  header.totalPackets = totalPackets;
  header.msgId = msgId;
  strncpy(header.pseudo, settings.pseudo, 15);
  header.pseudo[15] = '\0';
  header.priority = 1; // Priorité moyenne par défaut (à améliorer dans le menu)

  if (!radio.write(&header, sizeof(header))) {
    // Echec envoi header
    radio.startListening();
    return;
  }

  // 2. Envoyer Data (Fragmentation) [cite: 78, 79]
  for (int i = 0; i < totalPackets; i++) {
    DataPacket pkt;
    pkt.type = 1;
    pkt.seqNum = i;
    pkt.msgId = msgId;
    
    int startIdx = i * 28;
    String chunk = inputMessage.substring(startIdx, startIdx + 28);
    chunk.toCharArray(pkt.data, 28); // Copie safe
    
    delay(5); // Petit délai pour laisser le récepteur respirer
    radio.write(&pkt, sizeof(pkt));
  }

  radio.startListening();
  inputMessage = ""; // Reset message
}

// ---------------- MENUS ET AFFICHAGE ----------------
void drawTextCentered(String text, int y) {
  int16_t x1, y1; uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, y);
  display.print(text);
}

void runMenuLogic() {
  display.clearDisplay();
  
  if (currentState == MAIN_MENU) {
    display.setCursor(0, 0); display.println("--- BIP BIP ECE ---");
    const char* options[] = {"Ecrire Msg", "Reglages", "Dernier Msg"};
    
    menuIndex += getEncoderDelta();
    if (menuIndex < 0) menuIndex = 2;
    if (menuIndex > 2) menuIndex = 0;

    for (int i = 0; i < 3; i++) {
      if (i == menuIndex) display.print("> ");
      else display.print("  ");
      display.println(options[i]);
    }

    if (isEncoderButtonPressed()) {
      if (menuIndex == 0) currentState = WRITE_MSG;
      if (menuIndex == 1) currentState = SETTINGS;
      if (menuIndex == 2) currentState = READ_MSG;
      menuIndex = 0; encoderCount = 0;
    }
  } 
  else if (currentState == WRITE_MSG) {
    // Éditeur de texte style "SMS Nokia"
    static int charIndex = 0;
    charIndex += getEncoderDelta();
    if (charIndex < 0) charIndex = charMapLen - 1;
    if (charIndex >= charMapLen) charIndex = 0;

    display.setCursor(0, 0); display.print("Vers: TOUS");
    display.setCursor(0, 10); display.println(inputMessage);
    display.setCursor(0, 55); 
    display.print("Char: ["); display.print(charMap[charIndex]); display.print("]");

    if (isEncoderButtonPressed()) {
      inputMessage += charMap[charIndex]; // Ajoute le caractère
    }
    
    // Bouton A6 pour envoyer (EF4)
    if (isSendButtonPressed()) {
      display.clearDisplay();
      drawTextCentered("ENVOI...", 30);
      display.display();
      sendRadioMessage();
      delay(1000);
      currentState = MAIN_MENU;
    }
  }
  else if (currentState == SETTINGS) {
    const char* opts[] = {"Pseudo", "Canal", "Melodie", "Retour"};
    // Navigation simple... (à développer pour éditer chaque champ)
    // Ici, implémentation simplifiée pour l'exemple :
    display.setCursor(0,0); display.println("REGLAGES (Simu)");
    display.print("Canal: "); display.println(settings.canal);
    display.print("Pseudo: "); display.println(settings.pseudo);
    display.println("\n[Btn] pour retour");
    
    // Logique simplifiée : appui bouton = changer canal +1 pour test
    if (isEncoderButtonPressed()) {
      settings.canal = (settings.canal + 1) % 126;
      radio.setChannel(settings.canal);
      saveConfiguration(); // Sauvegarde (EF15)
    }
    if (isSendButtonPressed()) currentState = MAIN_MENU;
  }
  else if (currentState == READ_MSG || currentState == ALERT) {
    display.setCursor(0, 0); 
    display.print("De: "); display.println(receivedPseudo);
    display.println("---");
    display.println(receivedMsgBuffer);
    
    // Arrêter l'alerte si l'utilisateur appuie sur un bouton (EF8) [cite: 84]
    if (currentState == ALERT) {
       playTone(receivedPriority, settings.melody); // Joue le son en boucle (court)
       if (isEncoderButtonPressed() || isSendButtonPressed()) {
         stopAlert();
         currentState = READ_MSG; // Passe en mode lecture simple
       }
    } else {
       if (isSendButtonPressed()) currentState = MAIN_MENU;
    }
  }

  display.display();
}

// ---------------- SETUP & LOOP ----------------
void setup() {
  Serial.begin(115200);

  // Config Pins
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  // PIN_SEND_BUTTON (A6) est input analogique par défaut
  pinMode(PIN_LED_R, OUTPUT); pinMode(PIN_LED_G, OUTPUT); pinMode(PIN_LED_B, OUTPUT);
  
  // Important: Mettre D10 en OUTPUT LOW avant radio.begin pour éviter parasites buzzer 
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW);

  // Interrupt Encodeur
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderISR, CHANGE);

  // Init Ecran
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.cp437(true); // Active la page de code pour caractères étendus (accents) [cite: 72]
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();

  // Init EEPROM & Radio
  loadConfiguration(); // Récupère canal et pseudo [cite: 95]
  
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); // PA Low pour éviter problèmes d'alim en USB
  radio.setChannel(settings.canal); // EF12
  radio.openWritingPipe(pipeAddress);
  radio.openReadingPipe(1, pipeAddress);
  radio.startListening();
}

void loop() {
  // 1. Gestion Radio (Réception)
  if (radio.available()) {
    uint8_t size = radio.getPayloadSize();
    // Tampon générique
    uint8_t buf[32];
    radio.read(buf, size);

    if (buf[0] == 0) { // Header Packet
      HeaderPacket* h = (HeaderPacket*)buf;
      receivedPseudo = String(h->pseudo);
      receivedPriority = h->priority;
      receivedMsgBuffer = ""; // Reset buffer
    } else if (buf[0] == 1) { // Data Packet
      DataPacket* d = (DataPacket*)buf;
      receivedMsgBuffer += String(d->data);
      // Déclenche l'alerte à chaque réception de bout de message (ou attendre la fin)
      // Pour simplifier : on passe en mode alerte direct
      currentState = ALERT; // EF6, EF10 [cite: 81, 86]
    }
  }

  // 2. Gestion Interface Utilisateur
  runMenuLogic();
}