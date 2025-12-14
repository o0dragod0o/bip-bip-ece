#include "Config.h"
#include "Globals.h"
#include "Controle.h"
#include "Affichage.h"
#include "Sauvegarde.h"
#include "Radio.h"
#include "Navigation.h"

void setup() {
  // Toutes les initialisations
  initControl();
  initAffichage();
  loadSettings();
  initRadio();
  configurerRadio();
  
  // Initialisation menu
  currentMode = MainMenu;
  updateDisplay();
}

void loop() {
  ecouterRadio();

  //Gérer toute la logique (Menus, Ecriture, Reglages)
  handleNavigation();
}
