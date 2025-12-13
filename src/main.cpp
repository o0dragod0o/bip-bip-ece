/*
* PROJET BIP BIP ECE - VERSION MODULAIRE FINALE
*/

#include "Config.h"
#include "Globals.h"
#include "Controle.h"
#include "Affichage.h"
#include "Sauvegarde.h"
#include "Radio.h"
#include "Navigation.h"

void setup() {
  initControle();
  initAffichage();
  loadSettings();
  initRadio();
  configurerRadio();
  
  // Init menu
  currentMode = MODE_MENU_PRINCIPAL;
  updateDisplay();
}

void loop() {
  // 1. Tâche de fond : écouter la radio
  ecouterRadio();

  // 2. Gérer toute la logique (Menus, Ecriture, Reglages)
  gererNavigation();
}