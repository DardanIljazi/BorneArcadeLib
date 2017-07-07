// Slave exemple
#include "arcadeLib.h"


// Those variables are in the Borne_Arcade_Lib.cpp. We use "extern" to say that these variables are declared into an other file
extern volatile uint8_t lastButtonState;
extern volatile uint8_t lastJoystickState;
extern volatile bool    isInterruptActivated;

uint8_t invert = LOW;
uint8_t actualButtonState = 0x00;
uint8_t actualJoystickState = 0x00;

void setup() {

  if (initArcadeLib(SLAVE_MODE)) {
    // All is ok
  }
   
  pinMode(2, OUTPUT);
  digitalWrite(2, invert);
}

void loop() {
  // Get the button pin states
  actualButtonState = getButtonPinStates();
  // Get the joystick pin states
  actualJoystickState = getJoystickPinStates();

  // If there is something different between last and actual button states
  if (actualButtonState != lastButtonState) {
    noInterrupts(); // Use noInterrupts: We don't want this to be interrupted (by SPI communication)
    lastButtonState = actualButtonState;
    interrupts(); // We allow interrupts again

    // Change the pin state of pin 2 (if it was HIGH it will be LOW, if it was LOW it will be HIGH)
    // This is only done if 
    invert = (invert == LOW) ? HIGH : LOW;
    if(isInterruptActivated)
      digitalWriteFast(2, invert);
      
  }else if(actualJoystickState != lastJoystickState){
    noInterrupts();
    lastJoystickState = actualJoystickState ;
    interrupts();
    
    invert = (invert == LOW) ? HIGH : LOW;
    
    if(isInterruptActivated)
      digitalWriteFast(2, invert);
  }

}
