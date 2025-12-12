#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// ---------------- HARDWARE PINS -----------------
#define PIN_ENCODER_A 3
#define PIN_ENCODER_B 4
#define PIN_ENCODER_SW 2   // bouton de l'encodeur
#define PIN_SEND_BUTTON A6
#define PIN_LED_R 5
#define PIN_LED_G 6
#define PIN_LED_B 9
#define PIN_BUZZER 10
#define PIN_NRF_CE 7
#define PIN_NRF_CSN 8

// ---------------- GLOBAL VARIABLES ----------------
RF24 radio(PIN_NRF_CE, PIN_NRF_CSN);
byte address[6] = "CHAN1";
Adafruit_SSD1306 display(128, 64, &Wire,-1);

char pseudo[16] = "USER";
uint8_t canal = 76;

String message = "";
uint8_t priority = 1; // 0=high,1=medium,2=low

// Menu states
enum MenuState { MENU_MAIN, MENU_WRITE_MESSAGE, MENU_SETTINGS, MENU_EDIT_PSEUDO, MENU_EDIT_CANAL };
MenuState menuState = MENU_MAIN;
int8_t menuIndex = 0; // curseur dans menu ou paramètres

// ---------------- PACKET STRUCTURES ----------------
struct HeaderPacket {
  uint8_t id;
  uint8_t totalPackets;
  char pseudo[16];
  uint8_t priority;
};
struct DataPacket {
  uint8_t id;
  char data[30];
};

// ---------------- ENCODER ----------------
volatile int16_t encoderValue = 0;
void encoderISR() {
  if (digitalRead(PIN_ENCODER_A) == digitalRead(PIN_ENCODER_B)) encoderValue++;
  else encoderValue--;
}

// ---------------- BUTTONS ----------------
bool detectSendButton() {
  static bool last = false;
  bool cur = analogRead(PIN_SEND_BUTTON) < 200;
  bool pressed = (cur && !last);
  last = cur;
  return pressed;
}

bool detectEncoderButton() {
  static bool last = false;
  bool cur = digitalRead(PIN_ENCODER_SW) == LOW;
  bool pressed = (cur && !last);
  last = cur;
  return pressed;
}

// ---------------- RADIO SEND WITH ACK ----------------
bool sendLongMessage(const String &msg, const char *pseudo, uint8_t priority) {
  uint8_t totalPackets = (msg.length() + 29) / 30;

  HeaderPacket header;
  header.id = 0;
  header.totalPackets = totalPackets;
  header.priority = priority;
  strncpy(header.pseudo, pseudo, 15);
  header.pseudo[15] = '\0';

  radio.stopListening();
  bool success = radio.write(&header, sizeof(header));
  if (!success) return false;


  for (uint8_t i = 0; i < totalPackets; i++) {
    DataPacket pkt;
    pkt.id = i + 1;
    int start = i * 30;
    int len = min(30, (int)(msg.length() - start));
    memset(pkt.data, 0, 30);
    memcpy(pkt.data, msg.c_str() + start, len);
    if (!radio.write(&pkt, sizeof(pkt))) return false;
  }

  radio.startListening();
  return success;
}

// ---------------- RADIO RECEIVE ---------------------
String recvMessage = "";
String recvPseudo = "";
uint8_t recvPriority = 0;
uint8_t expectedPackets = 0;
uint8_t packetsReceived = 0;
bool buzzerActive = false;

void handleHeader(const HeaderPacket &h) {
  recvPseudo = h.pseudo;
  recvPriority = h.priority;
  expectedPackets = h.totalPackets;
  packetsReceived = 0;
  recvMessage = "";
}

void handleData(const DataPacket &p) {
  recvMessage += String(p.data);
  packetsReceived++;

  if (packetsReceived == expectedPackets) {
    buzzerActive = true;

    switch(recvPriority) {
      case 0: analogWrite(PIN_LED_R,255); analogWrite(PIN_LED_G,0); analogWrite(PIN_LED_B,0); break;
      case 1: analogWrite(PIN_LED_R,255); analogWrite(PIN_LED_G,255); analogWrite(PIN_LED_B,0); break;
      case 2: analogWrite(PIN_LED_R,0); analogWrite(PIN_LED_G,255); analogWrite(PIN_LED_B,0); break;
    }

    display.clearDisplay();
    display.setCursor(0,0);
    display.print("De: "); display.println(recvPseudo);
    display.print("Msg: "); display.println(recvMessage);
    display.display();
  }
}

// ---------------- MENU HANDLER ----------------
void handleMenu() {
  static char currentChar = 'A';

  display.clearDisplay();
  display.setCursor(0,0);

  // ---- MENU PRINCIPAL ----
  if(menuState == MENU_MAIN){
    display.println(menuIndex==0?"> Ecrire message":"  Ecrire message");
    display.println(menuIndex==1?"> Parametres":"  Parametres");
    display.display();

    if(encoderValue != 0){
      menuIndex = (menuIndex + (encoderValue > 0 ? 1 : -1) + 2) % 2;
      encoderValue = 0;
    }

    if(detectSendButton()){
      menuState = (menuIndex==0)? MENU_WRITE_MESSAGE : MENU_SETTINGS;
      menuIndex = 0; // curseur dans sous-menu
    }
  }

  // ---- ECRITURE MESSAGE ----
  else if(menuState == MENU_WRITE_MESSAGE){
    display.clearDisplay();
    display.setCursor(0,0); // <-- remise à zéro du curseur
    display.print("Message: "); display.println(message);
    display.print("Lettre: "); display.println(currentChar);
    display.display();

    // Tourner l'encodeur pour changer la lettre
    if(encoderValue != 0){
      currentChar += (encoderValue > 0 ? 1 : -1);
      if(currentChar > 126) currentChar = 32;
      if(currentChar < 32) currentChar = 126;
      encoderValue = 0;
    }

    // Bouton encodeur pour valider la lettre
    if(detectEncoderButton()){
      message += currentChar;
      currentChar = 'A';
      delay(100); // petit délai pour debounce bouton
    }

    // Bouton A6 pour envoyer
    if(detectSendButton() && message.length() > 0){
      bool ack = sendLongMessage(message, pseudo, priority);
      display.clearDisplay();
      display.setCursor(0,0);
      if(ack) display.println("Message recu");
      else display.println("Echec envoi");
      display.display();
      message = "";
      delay(1000);
      menuState = MENU_MAIN;
      menuIndex = 0;
    }
  }

  // ---- PARAMETRES ----
  else if(menuState == MENU_SETTINGS){
    display.println("Parametres:");
    display.print(menuIndex==0?"> ":"  "); display.print("Pseudo: "); display.println(pseudo);
    display.print(menuIndex==1?"> ":"  "); display.print("Canal: "); display.println(canal);
    display.display();

    // Naviguer entre Pseudo et Canal avec l'encodeur
    if(encoderValue != 0){
      menuIndex = (menuIndex + (encoderValue > 0 ? 1 : -1) + 2) % 2;
      encoderValue = 0;
    }

    // Entrer dans l'édition
    if(detectEncoderButton()){
      if(menuIndex==0){ // Pseudo
        for(int i=0;i<16;i++) pseudo[i]='\0';
        currentChar = 'A';
        menuState = MENU_EDIT_PSEUDO;
      }
      else if(menuIndex==1){ // Canal
        menuState = MENU_EDIT_CANAL;
      }
    }
    if(detectSendButton()) menuState = MENU_MAIN;
  }

  // ---- EDITION PSEUDO ----
  else if(menuState == MENU_EDIT_PSEUDO){
    display.print("Pseudo: "); display.println(pseudo);
    display.print("Lettre: "); display.println(currentChar);
    display.display();

    if(encoderValue != 0){
      currentChar += (encoderValue > 0 ? 1 : -1);
      if(currentChar > 126) currentChar = 32;
      if(currentChar < 32) currentChar = 126;
      encoderValue = 0;
    }

    if(detectEncoderButton()){
      for(int i=0;i<16;i++){
        if(pseudo[i]=='\0'){
          pseudo[i] = currentChar;
          pseudo[i+1] = '\0';
          break;
        }
      }
      currentChar = 'A';
    }

    if(detectSendButton()) menuState = MENU_SETTINGS;
  }

  // ---- EDITION CANAL ----
  else if(menuState == MENU_EDIT_CANAL){
    display.print("Canal: "); display.println(canal);
    display.display();

    if(encoderValue != 0){
      canal += (encoderValue > 0 ? 1 : -1);
      if(canal > 125) canal = 125;
      if(canal < 0) canal = 0;
      encoderValue = 0;
    }

    if(detectSendButton()) menuState = MENU_SETTINGS;
  }
}

// ------------------- SETUP -------------------------
void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_ENCODER_A, INPUT_PULLUP);
  pinMode(PIN_ENCODER_B, INPUT_PULLUP);
  pinMode(PIN_ENCODER_SW, INPUT_PULLUP);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), encoderISR, CHANGE);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.display();

  radio.begin();
  radio.setChannel(canal);
  radio.openWritingPipe(address);
  radio.openReadingPipe(1, address);
  radio.startListening();
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.setRetries(5, 15);
}

// ------------------- LOOP --------------------------
void loop() {
  handleMenu();

  // ---------------- RADIO RECEIVE ----------------
  if(radio.available()){
    uint8_t size = radio.getPayloadSize();
    if(size == sizeof(HeaderPacket)){
      HeaderPacket h;
      radio.read(&h,sizeof(h));
      handleHeader(h);
    } else if(size == sizeof(DataPacket)){
      DataPacket d;
      radio.read(&d,sizeof(d));
      handleData(d);
    } else {
      char buf[10];
      radio.read(&buf,size);
      if(strcmp(buf,"ACK")==0){
        display.clearDisplay();
        display.setCursor(0,0);
        display.println("Message recu ✔");
        display.display();
      } else radio.flush_rx();
    }
  }

  // ---------------- BUZZER ----------------
  if(buzzerActive){
    analogWrite(PIN_BUZZER,50); // moins fort, continu
    if(detectSendButton()) buzzerActive = false; // stop buzzer
  } else {
    analogWrite(PIN_BUZZER,0);
  }

  delay(50);
}