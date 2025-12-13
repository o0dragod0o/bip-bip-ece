#include "Navigation.h"
#include "Controle.h"
#include "Affichage.h"
#include "Sauvegarde.h"
#include "Radio.h"

// --- FONCTIONS LOCALES ---
void lettreSuivante() {
  if (currentLetter == 'Z') currentLetter = '0'; 
  else if (currentLetter == '9') currentLetter = ' '; 
  else if (currentLetter == ' ') currentLetter = 'A'; 
  else currentLetter++;
}

void lettrePrecedente() {
  if (currentLetter == 'A') currentLetter = ' '; 
  else if (currentLetter == ' ') currentLetter = '9'; 
  else if (currentLetter == '0') currentLetter = 'Z'; 
  else currentLetter--;
}

// --- LOGIQUE PRINCIPALE ---
void gererNavigation() {
  // 1. Lire toutes les entrées une seule fois au début
  int dir = lireEncodeurDir(); // -1, 0, ou 1
  bool click = lireBoutonSW(); // Vrai si cliqué
  bool a6 = lireBoutonA6();    // Vrai si appuyé

  // 2. Machine à état
  switch (currentMode) {

    // ====================================================
    // 1. MENU PRINCIPAL
    // ====================================================
    case MODE_MENU_PRINCIPAL:
      if (dir != 0) { 
        menuSelection = !menuSelection; // Bascule 0 <-> 1
        updateDisplay(); 
      }
      if (click) { 
        if (menuSelection == 0) { 
          currentMode = MODE_ECRITURE; 
          cursorPosition = 0; 
          memset(sharedBuffer, 0, MAX_MESSAGE_LEN); 
        } else { 
          currentMode = MODE_MENU_REGLAGE; 
          menuSelection = 0; 
        }
        updateDisplay();
      }
      break;

    // ====================================================
    // 2. MENU REGLAGES (Navigation Verticale)
    // ====================================================
    case MODE_MENU_REGLAGE:
      if (dir != 0) {
         if(dir > 0) { // Descente
            menuSelection++; 
            if(menuSelection > 3) menuSelection = 0;
         } else {      // Montée
            menuSelection--; 
            if(menuSelection < 0) menuSelection = 3;
         }
         updateDisplay();
      }
      if (click) {
         if (menuSelection == 0) {      // PSEUDO
            currentMode = MODE_EDIT_PSEUDO; 
            cursorPosition = strlen(myPseudo); 
            if(cursorPosition >= PSEUDO_LEN - 1) cursorPosition = 0; // Sécurité
            currentLetter = 'A'; 
         } 
         else if (menuSelection == 1) currentMode = MODE_EDIT_CANAL;
         else if (menuSelection == 2) currentMode = MODE_EDIT_SLOT;
         else if (menuSelection == 3) currentMode = MODE_EDIT_SOUND;
         updateDisplay();
      }
      if (a6) { // RETOUR
         currentMode = MODE_MENU_PRINCIPAL; 
         updateDisplay(); 
         delay(400); 
      }
      break;

    // ====================================================
    // 3. ECRITURE MESSAGE (Fonctionne déjà, on garde)
    // ====================================================
    case MODE_ECRITURE:
      if (dir != 0) { 
         if(dir > 0) lettreSuivante(); 
         else lettrePrecedente(); 
         updateDisplay(); 
      }
      if (click) { 
         if (cursorPosition < MAX_MESSAGE_LEN-1) { 
            sharedBuffer[cursorPosition] = currentLetter; 
            cursorPosition++; 
            sharedBuffer[cursorPosition] = '\0'; 
         }
         updateDisplay();
      }
      if (a6) { // FINI -> CHOIX PRIORITE
         currentMode = MODE_CHOIX_PRIORITE; 
         selectedPriority = PRIO_FAIBLE; 
         updateDisplay(); 
         delay(500); 
      }
      break;

    // ====================================================
    // 4. CHOIX PRIORITE (C'était bloqué ici)
    // ====================================================
    case MODE_CHOIX_PRIORITE:
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
          currentMode = MODE_MENU_PRINCIPAL; 
          setLedColor(255);           // Eteindre LED
          updateDisplay();
          delay(500);
      }
      // Click molette pour ANNULER et revenir à l'écriture
      if (click) {
          currentMode = MODE_ECRITURE;
          updateDisplay();
      }
      break;

    // ====================================================
    // 5. EDITER PSEUDO
    // ====================================================
    case MODE_EDIT_PSEUDO:
      if (dir != 0) { 
         if(dir > 0) lettreSuivante(); 
         else lettrePrecedente(); 
         updateDisplay(); 
      }
      if (click) { // Lettre suivante
         if (cursorPosition < PSEUDO_LEN - 1) {
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
         currentMode = MODE_MENU_REGLAGE;
         updateDisplay();
         delay(400);
      }
      break;

    // ====================================================
    // 6. EDITER CANAL
    // ====================================================
    case MODE_EDIT_CANAL:
      if (dir != 0) {
         if (dir > 0) { if (radioChannel < 125) radioChannel++; }
         else         { if (radioChannel > 0) radioChannel--; }
         
         configurerRadio(); // Applique le changement immédiatement pour tester
         updateDisplay();
      }
      if (click || a6) { // VALIDER
         saveSettingsAll();
         currentMode = MODE_MENU_REGLAGE;
         updateDisplay();
         delay(400);
      }
      break;

    // ====================================================
    // 7. EDITER SLOT (MODE RADIO A/B)
    // ====================================================
    case MODE_EDIT_SLOT:
      if (dir != 0) {
         radioSlot = !radioSlot; // Bascule simplement 0 ou 1
         updateDisplay();
      }
      if (click || a6) { // VALIDER
         saveSettingsAll();
         configurerRadio(); // Important de reconfigurer les tuyaux radio
         currentMode = MODE_MENU_REGLAGE;
         updateDisplay();
         delay(400);
      }
      break;

    // ====================================================
    // 8. EDITER SONORITE
    // ====================================================
    case MODE_EDIT_SOUND:
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
         currentMode = MODE_MENU_REGLAGE;
         updateDisplay();
         delay(400);
      }
      break;

    // ====================================================
    // 9. ALERTE RECU
    // ====================================================
    case MODE_ALERTE_RECU:
       gererAlarmeSonore(); // Doit être appelé en boucle
       
       if (a6) { // ACQUITTER
          setLedColor(255); 
          digitalWrite(PIN_BUZZER, LOW); 
          currentMode = MODE_MENU_PRINCIPAL; 
          updateDisplay(); 
          delay(500); 
       }
       break;
  }
}