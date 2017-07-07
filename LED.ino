// MASTER Blink exemple
// Inclure la librairie
#include "arcadeLib.h"

#define SS 5
#define LED_PIN 3

void setup (void)
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if(initArcadeLib(MASTER_MODE, SS)){ // Activer le mode MASTER_MODE avec le SS pin sur le numéro 5
    if(communicationTest()){
      // Communication Ok
      digitalWrite(LED_PIN, HIGH); // On met la led en état HAUT pour signifier que la communication a bien fonctionnée
    }else{
     // Impossible d'initialiser la communication (erreur)
    }
  }else{
    // Impossible d'initialiser la librairie
  }
  delay(1000);
  digitalWrite(LED_PIN, LOW); // On éteint la LED
}

void loop() {
  getActualButtonStates(); // On récupère le dernier état des boutons
  getActualJoystickStates(); // On récupère le dernier état du joystick

  if(isStartButtonPressed()){ // Si le bouton est appuyé, on allume la led, sinon on l'éteint
    digitalWrite(LED_PIN, HIGH);
  }else{
    digitalWrite(LED_PIN, LOW);
  }
  
}
