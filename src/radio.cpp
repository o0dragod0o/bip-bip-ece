#include "Radio.h"
#include "Affichage.h" // Besoin pour updateDisplay() et setLedColor()

void initRadio() {
  if (!radio.begin()) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("Erreur Radio !"));
    display.display();
    while (1);
  }
  radio.setPALevel(RF24_PA_MIN); 
}

void configurerRadio() {
  radio.stopListening();
  radio.setChannel(radioChannel);
  
  if (radioSlot == 0) { // MODE A
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);
  } else {              // MODE B
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1, pipes[1]);
  }
  radio.startListening();
}

void envoyerMessageLong() {
  radio.stopListening();
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(1);
  display.println(F("Envoi..."));
  display.display(); 

  setLedColor(255); delay(50);
  setLedColor(selectedPriority); delay(100);
  setLedColor(255);

  int msgLen = strlen(sharedBuffer);
  int totalPackets = (msgLen / PACKET_DATA_SIZE);
  if ((msgLen % PACKET_DATA_SIZE) != 0) totalPackets++;
  if (totalPackets == 0) totalPackets = 1; 
  currentMsgId++;

  for (int i = 0; i < totalPackets; i++) {
    Packet pkt;
    pkt.msgId = currentMsgId;
    pkt.packetIndex = i;
    pkt.totalPackets = totalPackets;
    pkt.priority = selectedPriority; 
    strcpy(pkt.senderName, myPseudo); 
    memset(pkt.payload, 0, PACKET_DATA_SIZE);

    int charsToCopy = PACKET_DATA_SIZE;
    int startIndex = i * PACKET_DATA_SIZE;
    if (startIndex + charsToCopy > msgLen) charsToCopy = msgLen - startIndex;
    if (charsToCopy > 0) memcpy(pkt.payload, &sharedBuffer[startIndex], charsToCopy);

    radio.write(&pkt, sizeof(pkt));
    delay(40); 
  }
  radio.startListening();
}

void ecouterRadio() {
  if (radio.available()) {
    bool messageComplet = false;
    while (radio.available()) { 
      Packet pkt;
      radio.read(&pkt, sizeof(pkt));
      receivedPriority = pkt.priority;
      strcpy(receivedPseudo, pkt.senderName); 

      int offset = pkt.packetIndex * PACKET_DATA_SIZE;
      if (offset < MAX_MESSAGE_LEN) {
        int bytesToCopy = PACKET_DATA_SIZE;
        if (offset + bytesToCopy > MAX_MESSAGE_LEN) bytesToCopy = MAX_MESSAGE_LEN - offset;
        memcpy(sharedBuffer + offset, pkt.payload, bytesToCopy);
        
        if (pkt.packetIndex == pkt.totalPackets - 1) {
          sharedBuffer[MAX_MESSAGE_LEN - 1] = '\0'; 
          messageComplet = true;
        }
      }
    }
    if (messageComplet) {
      currentMode = MODE_ALERTE_RECU;
      buzzerTimer = millis(); 
      buzzerStep = 0;
      updateDisplay(); 
      setLedColor(receivedPriority);
    }
  }
}