#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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
#define MAGIC_BYTE    101 // Changé

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

// --- ENUMS & STRUCTS ---
#define PRIO_FAIBLE 0 
#define PRIO_MOYEN  1 
#define PRIO_HAUTE  2 

#define SON_CLASSIQUE 0
#define SON_DOUBLE    1
#define SON_CONTINU   2

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

#endif