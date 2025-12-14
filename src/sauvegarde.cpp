#include "Sauvegarde.h"

void loadSettings() {
  byte marker = EEPROM.read(AddrMarker);
  if (marker != MagicByte) {
    radioChannel = 76;
    radioSlot = 0; 
    alertSound = ClassicSound; 
    strcpy(myPseudo, "USER");
    saveSettingsAll();
    EEPROM.write(AddrMarker, MagicByte);
  } else {
    radioChannel = EEPROM.read(AddrChannel);
    radioSlot = EEPROM.read(AddrSlot);
    alertSound = EEPROM.read(AddrSound);
    if(alertSound > 2) alertSound = 0; 
    EEPROM.get(AddrPseudo, myPseudo);
  }
}

void saveSettingsAll() {
  EEPROM.update(AddrChannel, radioChannel);
  EEPROM.update(AddrSlot, radioSlot);
  EEPROM.update(AddrSound, alertSound);
  EEPROM.put(AddrPseudo, myPseudo);
}