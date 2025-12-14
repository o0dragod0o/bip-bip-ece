#include "Config.h"
#include "Globals.h"
#include "Controle.h"
#include "Affichage.h"
#include "Sauvegarde.h"
#include "Radio.h"
#include "Navigation.h"

void setup() {
  initControl();
  initAffichage();
  loadSettings();
  initRadio();
  configurerRadio();
  
  // Init menu
  currentMode = MainMenu;
  updateDisplay();
}

void loop() {
  // 1. Tâche de fond : écouter la radio
  ecouterRadio();

  // 2. Gérer toute la logique (Menus, Ecriture, Reglages)
  handleNavigation();
}