#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- CONSTANTES ---
#define MaxMessageLen 120
#define PseudoLen 6          
#define PacketDataSize 22   

// --- ADRESSES EEPROM ---
#define AddrMarker   0   
#define AddrChannel  1   
#define AddrSlot     2   
#define AddrSound    3   
#define AddrPseudo   10  
#define MagicByte    101 // Changé

// --- PINS ---
#define PIN_CE 7
#define PIN_CSN 8
#define BuzzerPin 10
#define RedPin 5
#define GreenPin 6
#define BluePin 9
#define EncCLK 3       
#define ENC_DT 4       
#define EncSW 2 
#define SendBtn A6    

// --- ECRAN ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 

// --- ENUMS & STRUCTS ---
#define LowPrio 0 
#define AveragePrio  1 
#define HighPrio  2 

#define ClassicSound 0
#define DoubleSound    1
#define EndlessSound   2

struct Packet {
  byte msgId;         
  byte packetIndex;   
  byte totalPackets;
  byte priority;      
  char senderName[PseudoLen]; 
  char payload[PacketDataSize]; 
};

enum Mode {
  MainMenu,    
  SettingsMenu,      
  PseudoMode,       
  CanalEditMode,
  SlotEditMode,
  SoundEditMode,        
  WritingMode,          
  PriorityMode,    
  AlarmMode        
};

#endif
