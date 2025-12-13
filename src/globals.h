#ifndef GLOBALS_H
#define GLOBALS_H

#include "Config.h"
#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;
extern RF24 radio;
extern const byte pipes[][6];

extern char currentLetter; 
extern char sharedBuffer[MAX_MESSAGE_LEN]; 
extern char myPseudo[PSEUDO_LEN]; 
extern char receivedPseudo[PSEUDO_LEN];     

extern int cursorPosition; 
extern byte currentMsgId; 
extern byte selectedPriority; 
extern byte receivedPriority;

extern byte radioChannel; 
extern byte radioSlot; 
extern byte alertSound; 

extern Mode currentMode;
extern int menuSelection; 
extern unsigned long buzzerTimer;
extern int buzzerStep;

#endif