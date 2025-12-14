#include "Globals.h"

// Création réelle des objets et variables en mémoire
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RF24 radio(PIN_CE, PIN_CSN);
const byte pipes[][6] = {"00001", "00002"};

char currentLetter = 'A'; 
char sharedBuffer[MaxMessageLen]; 
char myPseudo[PseudoLen]; 
char receivedPseudo[PseudoLen];     

int cursorPosition = 0; 
byte currentMsgId = 0; 
byte selectedPriority = LowPrio; 
byte receivedPriority = LowPrio;

byte radioChannel; 
byte radioSlot; 
byte alertSound;

Mode currentMode = MainMenu;
int selectionMenu = 0;
unsigned long buzzerTimer = 0;
int buzzerStep = 0;