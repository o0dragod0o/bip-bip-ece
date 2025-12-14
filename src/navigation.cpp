#include "Navigation.h"
#include "Controle.h"
#include "Affichage.h"
#include "Sauvegarde.h"
#include "Radio.h"

//FONCTIONS LOCALES
void nextLetter() {
  if (currentLetter == 'Z') currentLetter = '0'; 
  else if (currentLetter == '9') currentLetter = ' '; 
  else if (currentLetter == ' ') currentLetter = 'A'; 
  else currentLetter++;
}

void prevLetter() {
  if (currentLetter == 'A') currentLetter = ' '; 
  else if (currentLetter == ' ') currentLetter = '9'; 
  else if (currentLetter == '0') currentLetter = 'Z'; 
  else currentLetter--;
}

//LOGIQUE PRINCIPALE
void handleNavigation() {
  // 1. Lire toutes les entrées une seule fois au début
  int dir = readEncDir(); // -1, 0, ou 1
  bool click = readSWBtn(); // Vrai si cliqué
  bool a6 = readSendBtn();    // Vrai si appuyé

  // 2. Machine à état
  switch (currentMode) {
    // 1. MENU PRINCIPAL
    case MainMenu:
      if (dir != 0) { 
        selectionMenu = !selectionMenu; // Bascule 0 <-> 1
        updateDisplay(); 
      }
      if (click) { 
        if (selectionMenu == 0) { 
          currentMode = WritingMode; 
          cursorPosition = 0; 
          memset(sharedBuffer, 0, MaxMessageLen); 
        } else { 
          currentMode = SettingsMenu; 
          selectionMenu = 0; 
        }
        updateDisplay();
      }
      break;
    // 2. MENU REGLAGES (Navigation Verticale)
    case SettingsMenu:
      if (dir != 0) {
         if(dir > 0) { // Descente
            selectionMenu++; 
            if(selectionMenu > 3) selectionMenu = 0;
         } else {      // Montée
            selectionMenu--; 
            if(selectionMenu < 0) selectionMenu = 3;
         }
         updateDisplay();
      }
      if (click) {
         if (selectionMenu == 0) {      // PSEUDO
            currentMode = PseudoMode; 
            cursorPosition = strlen(myPseudo); 
            if(cursorPosition >= PseudoLen - 1) cursorPosition = 0; // Sécurité
            currentLetter = 'A'; 
         } 
         else if (selectionMenu == 1) currentMode = CanalEditMode;
         else if (selectionMenu == 2) currentMode = SlotEditMode;
         else if (selectionMenu == 3) currentMode = SoundEditMode;
         updateDisplay();
      }
      if (a6) { // RETOUR
         currentMode = MainMenu; 
         updateDisplay(); 
         delay(400); 
      }
      break;
    // 3. ECRITURE MESSAGE (Fonctionne déjà, on garde)
    case WritingMode:
      if (dir != 0) { 
         if(dir > 0) nextLetter(); 
         else prevLetter(); 
         updateDisplay(); 
      }
      if (click) { 
         if (cursorPosition < MaxMessageLen-1) { 
            sharedBuffer[cursorPosition] = currentLetter; 
            cursorPosition++; 
            sharedBuffer[cursorPosition] = '\0'; 
         }
         updateDisplay();
      }
      if (a6) { // CHOIX PRIORITE
         currentMode = PriorityMode; 
         selectedPriority = LowPrio; 
         updateDisplay(); 
         delay(500); 
      }
      break;
    // 4. CHOIX PRIORITE
    case PriorityMode:
      if (dir != 0) {
         if (dir > 0) { // Rotation Droite
            selectedPriority++;
            if (selectedPriority > 2) selectedPriority = 0;
         } else {       // Rotation Gauche
            if (selectedPriority == 0) selectedPriority = 2;
            else selectedPriority--;
         }
         updateDisplay(); 
      }
      
      // Ici, A6 sert à ENVOYER
      if (a6) {
          envoyerMessageLong();       // Action d'envoi
          currentMode = MainMenu; 
          setLedColor(255);           // Eteindre LED
          updateDisplay();
          delay(500);
      }
      // appui bouton SW pour ANNULER et revenir à l'écriture
      if (click) {
          currentMode = WritingMode;
          updateDisplay();
      }
      break;
    // 5. EDITER PSEUDO
    case PseudoMode:
      if (dir != 0) { 
         if(dir > 0) nextLetter(); 
         else prevLetter(); 
         updateDisplay(); 
      }
      if (click) { // Lettre suivante
         if (cursorPosition < PseudoLen - 1) {
            myPseudo[cursorPosition] = currentLetter;
            cursorPosition++;
            myPseudo[cursorPosition] = '\0';
         } else {
            cursorPosition = 0; // Boucle au début si on est au bout
         }
         updateDisplay();
      }
      if (a6) { // SAUVEGARDER ET QUITTER
         saveSettingsAll();
         currentMode = SettingsMenu;
         updateDisplay();
         delay(400);
      }
      break;
    // 6. EDITER CANAL
    case CanalEditMode:
      if (dir != 0) {
         if (dir > 0) { if (radioChannel < 125) radioChannel++; }
         else         { if (radioChannel > 0) radioChannel--; }
         
         configurerRadio(); // Applique le changement immédiatement pour tester
         updateDisplay();
      }
      if (click || a6) { // VALIDER
         saveSettingsAll();
         currentMode = SettingsMenu;
         updateDisplay();
         delay(400);
      }
      break;
    // 7. EDITER SLOT (MODE RADIO A/B)
    case SlotEditMode:
      if (dir != 0) {
         radioSlot = !radioSlot; // Bascule simplement 0 ou 1
         updateDisplay();
      }
      if (click || a6) { // VALIDER
         saveSettingsAll();
         configurerRadio(); // Important de reconfigurer les tuyaux radio
         currentMode = SettingsMenu;
         updateDisplay();
         delay(400);
      }
      break;
    // 8. EDITER SONORITE
    case SoundEditMode:
      if (dir != 0) {
         if (dir > 0) { 
            alertSound++; 
            if(alertSound > 2) alertSound = 0; 
         } else { 
            if(alertSound == 0) alertSound = 2; 
            else alertSound--; 
         }
         updateDisplay();
         previewSound(); // Petit bip pour entendre le choix
      }
      if (click || a6) { // VALIDER
         saveSettingsAll();
         currentMode = SettingsMenu;
         updateDisplay();
         delay(400);
      }
      break;
    // 9. ALERTE RECU
    case AlarmMode:
       handleAlarm(); // Doit être appelé en boucle
       
       if (a6) { // ACQUITTER
          setLedColor(255); 
          digitalWrite(BuzzerPin, LOW); 
          currentMode = MainMenu; 
          updateDisplay(); 
          delay(500); 
       }
       break;
  }
}
