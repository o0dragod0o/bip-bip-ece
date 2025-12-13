#include "Sauvegarde.h"

void loadSettings() {
  byte marker = EEPROM.read(ADDR_MARKER);
  if (marker != MAGIC_BYTE) {
    // Valeurs par défaut
    radioChannel = 76;
    radioSlot = 0; 
    alertSound = SON_CLASSIQUE; 
    strcpy(myPseudo, "USER");
    saveSettingsAll();
    EEPROM.write(ADDR_MARKER, MAGIC_BYTE);
  } else {
    radioChannel = EEPROM.read(ADDR_CHANNEL);
    radioSlot = EEPROM.read(ADDR_SLOT);
    alertSound = EEPROM.read(ADDR_SOUND);
    if(alertSound > 2) alertSound = 0; 
    EEPROM.get(ADDR_PSEUDO, myPseudo);
  }
}

void saveSettingsAll() {
  EEPROM.update(ADDR_CHANNEL, radioChannel);
  EEPROM.update(ADDR_SLOT, radioSlot);
  EEPROM.update(ADDR_SOUND, alertSound);
  EEPROM.put(ADDR_PSEUDO, myPseudo);
}