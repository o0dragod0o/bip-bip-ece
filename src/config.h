#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// --- CONSTANTES ---
#define MAX_MESSAGE_LEN 100   
#define PSEUDO_LEN 6          
#define PACKET_DATA_SIZE 22   

// --- ADRESSES EEPROM ---
#define ADDR_MARKER   0   
#define ADDR_CHANNEL  1   
#define ADDR_SLOT     2   
#define ADDR_SOUND    3   
#define ADDR_PSEUDO   10  
#define MAGIC_BYTE    99  

// --- PRIORITÉS & SONS ---
#define PRIO_FAIBLE 0 
#define PRIO_MOYEN  1 
#define PRIO_HAUTE  2 

#define SON_CLASSIQUE 0
#define SON_DOUBLE    1
#define SON_CONTINU   2

// --- PINS ---
#define PIN_CE 7
#define PIN_CSN 8
#define PIN_BUZZER 10
#define PIN_RED 5
#define PIN_GREEN 6
#define PIN_BLUE 9
#define ENC_CLK 3       
#define ENC_DT 4       
#define ENC_SW 2       
#define BTN_SEND A6    

// --- ECRAN ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 

// --- STRUCTURES & ENUMS ---
struct Packet {
  byte msgId;         
  byte packetIndex;   
  byte totalPackets;
  byte priority;      
  char senderName[PSEUDO_LEN]; 
  char payload[PACKET_DATA_SIZE]; 
};

enum Mode {
  MODE_MENU_PRINCIPAL,    
  MODE_MENU_REGLAGE,      
  MODE_EDIT_PSEUDO,       
  MODE_EDIT_CANAL,
  MODE_EDIT_SLOT,
  MODE_EDIT_SOUND,        
  MODE_ECRITURE,          
  MODE_CHOIX_PRIORITE,    
  MODE_ALERTE_RECU        
};

// --- VARIABLES GLOBALES PARTAGÉES (EXTERN) ---
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