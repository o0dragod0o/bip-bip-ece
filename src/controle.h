#ifndef CONTROLE_H
#define CONTROLE_H
#include "Config.h"

void initControle();
int lireEncodeurDir(); // Retourne -1 (Gauche), 0 (Rien), 1 (Droite)
bool lireBoutonSW();   // Retourne true si cliqué
bool lireBoutonA6();   // Retourne true si appuyé

#endif