#include "Radio.h"
#include "Affichage.h"

void initRadio() {
  if (!radio.begin()) { while (1); } // Bloque si erreur
  radio.setPALevel(RF24_PA_MIN); 
}

void configurerRadio() {
  radio.stopListening();
  radio.setChannel(radioChannel);
  if (radioSlot == 0) { radio.openWritingPipe(pipes[1]); radio.openReadingPipe(1, pipes[0]); } 
  else { radio.openWritingPipe(pipes[0]); radio.openReadingPipe(1, pipes[1]); }
  radio.startListening();
}

void envoyerMessageLong() {
  radio.stopListening();
  display.clearDisplay(); display.setCursor(0, 20); display.println(F("Envoi...")); display.display(); 
  
  setLedColor(selectedPriority); delay(100); setLedColor(255);

  int msgLen = strlen(sharedBuffer);
  int totalPackets = (msgLen / PACKET_DATA_SIZE);
  if ((msgLen % PACKET_DATA_SIZE) != 0) totalPackets++;
  if (totalPackets == 0) totalPackets = 1; 
  currentMsgId++;

  for (int i = 0; i < totalPackets; i++) {
    Packet pkt;
    pkt.msgId = currentMsgId; pkt.packetIndex = i; pkt.totalPackets = totalPackets;
    pkt.priority = selectedPriority; strcpy(pkt.senderName, myPseudo); 
    memset(pkt.payload, 0, PACKET_DATA_SIZE);
    
    int bytes = PACKET_DATA_SIZE;
    int start = i * PACKET_DATA_SIZE;
    if (start + bytes > msgLen) bytes = msgLen - start;
    if (bytes > 0) memcpy(pkt.payload, &sharedBuffer[start], bytes);

    radio.write(&pkt, sizeof(pkt));
    delay(40); 
  }
  radio.startListening();
}

void ecouterRadio() {
  if (radio.available()) {
    bool complet = false;
    while (radio.available()) { 
      Packet pkt; radio.read(&pkt, sizeof(pkt));
      receivedPriority = pkt.priority; strcpy(receivedPseudo, pkt.senderName); 
      int offset = pkt.packetIndex * PACKET_DATA_SIZE;
      if (offset < MAX_MESSAGE_LEN) {
         memcpy(sharedBuffer + offset, pkt.payload, (offset + PACKET_DATA_SIZE > MAX_MESSAGE_LEN) ? MAX_MESSAGE_LEN - offset : PACKET_DATA_SIZE);
         if (pkt.packetIndex == pkt.totalPackets - 1) { sharedBuffer[MAX_MESSAGE_LEN - 1] = '\0'; complet = true; }
      }
    }
    if (complet) {
      currentMode = MODE_ALERTE_RECU;
      buzzerTimer = millis(); buzzerStep = 0;
      updateDisplay(); setLedColor(receivedPriority);
    }
  }
}