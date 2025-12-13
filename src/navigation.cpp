#include "Navigation.h"
#include "Controle.h"
#include "Affichage.h"
#include "Sauvegarde.h"
#include "Radio.h"

// Helpers texte
void lettreSuivante() {
  if (currentLetter == 'Z') currentLetter = '0'; else if (currentLetter == '9') currentLetter = ' '; else if (currentLetter == ' ') currentLetter = 'A'; else currentLetter++;
}
void lettrePrecedente() {
  if (currentLetter == 'A') currentLetter = ' '; else if (currentLetter == ' ') currentLetter = '9'; else if (currentLetter == '0') currentLetter = 'Z'; else currentLetter--;
}

void gererNavigation() {
  // 1. Lire les entrées une seule fois
  int dir = lireEncodeurDir();
  bool click = lireBoutonSW();
  bool a6 = lireBoutonA6();

  // 2. Machine à état
  switch (currentMode) {
    case MODE_MENU_PRINCIPAL:
      if (dir != 0) { menuSelection = !menuSelection; updateDisplay(); }
      if (click) { 
        if (menuSelection == 0) { currentMode = MODE_ECRITURE; cursorPosition = 0; memset(sharedBuffer, 0, MAX_MESSAGE_LEN); } 
        else { currentMode = MODE_MENU_REGLAGE; menuSelection = 0; }
        updateDisplay();
      }
      break;

    case MODE_MENU_REGLAGE:
      if (dir != 0) {
         if(dir > 0) { menuSelection++; if(menuSelection > 3) menuSelection=0; }
         else { menuSelection--; if(menuSelection < 0) menuSelection=3; }
         updateDisplay();
      }
      if (click) {
         if (menuSelection == 0) { currentMode = MODE_EDIT_PSEUDO; cursorPosition = strlen(myPseudo); currentLetter = 'A'; }
         else if (menuSelection == 1) currentMode = MODE_EDIT_CANAL;
         else if (menuSelection == 2) currentMode = MODE_EDIT_SLOT;
         else if (menuSelection == 3) currentMode = MODE_EDIT_SOUND;
         updateDisplay();
      }
      if (a6) { currentMode = MODE_MENU_PRINCIPAL; updateDisplay(); delay(400); }
      break;

    case MODE_ECRITURE:
      if (dir != 0) { if(dir>0) lettreSuivante(); else lettrePrecedente(); updateDisplay(); }
      if (click) { 
         if (cursorPosition < MAX_MESSAGE_LEN-1) { sharedBuffer[cursorPosition] = currentLetter; cursorPosition++; sharedBuffer[cursorPosition] = '\0'; }
         updateDisplay();
      }
      if (a6) { currentMode = MODE_CHOIX_PRIORITE; selectedPriority = PRIO_FAIBLE; updateDisplay(); delay(500); }
      break;
      
    // ... AJOUTEZ ICI LES AUTRES CAS (EDIT_CANAL, EDIT_SLOT, etc.)
    // La logique est : 
    // Si (dir != 0) -> modifier valeur -> updateDisplay
    // Si (a6) -> Sauvegarder -> updateDisplay
    
    case MODE_ALERTE_RECU:
       gererAlarmeSonore();
       if (a6) { setLedColor(255); digitalWrite(PIN_BUZZER, LOW); currentMode = MODE_MENU_PRINCIPAL; updateDisplay(); delay(500); }
       break;
  }
}