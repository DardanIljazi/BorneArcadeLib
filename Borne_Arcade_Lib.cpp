#include "arcadeLib.h"

/**
  For the brave souls who get this far: You are the chosen ones,
  the valiant knights of programming who toil away, without rest,
  fixing our most awful code. To you, true saviors, kings of men,
  I say this: never gonna give you up, never gonna let you down,
  never gonna run around and desert you. Never gonna make you cry,
  never gonna say goodbye. Never gonna tell a lie and hurt you.
*/

/**
	TODO: 	Some functions are the "same" in their structure. (lie isStartButtonPressed(), isStopButtonPressed() aso..)\
			Those functions should be suppressed in the future to integrate them into one single function.
	TODO:   This library has been developed in C (Arduino) with global variables, which is very bad. All the library should be reprogramed on C++ with OOP.
	INFORMATION: This library have been tested with 2 Arduinos on very small distance between them, like wires of 10-15cm maximum. On longer wires, impedance could make it not work well, not tested.
*/

/*
   General global variable
   Those variables are used both in Master or Slave mode 
*/
Mode actualMode; /**< actualMode is a variable used in 2 states: SLAVE_MODE or MASTER_MODE. It is set in the initArcadeLib(int mode) function. This avoid function miscall (call a Slave function on Master mode or Master function on Slave mode).*/

unsigned long lastTimeButton, lastTimeJoystick;  /**<  To avoid bounce on button press, there is a time of 10 ms between each button press. Those variables below are the last time a button have been pressed */
/*--------------------------------------------*/

/*
    *MASTER* mode global variable
	Those variables are used into Master mode only
*/

int SSPin = 10; /**<  SSPin refers to Slave Select pin used on SPI protocol communication. By default, the pin 10 is used. This can be changed. */

volatile uint8_t buttonStatesMaster   = 0x00, joystickStatesMaster = 0x00; /** Those variables are used in Master mode. It is the result of Master's request for button and joystick states.
   These are volatile because they are shared between Interrupts function (getActualButtonState) and others like isStartButtonPressed(), isButton1Pressed()... To know more about interrupts, please visit http://gammon.com.au/interrupts */

InterruptMode actualInterruptMode = NORMAL_INTERRUPT_MODE; /**< This variable is used when interrupts are called. There is 2 modes: NORMAL_MODE (nothing special) and REMAIN_HIGH_UNTIL_NEXTCALL. This last one */

/*!
   The first variable functionPointer (below), can contain 12 pointer to void function
   The second variable, functionPointerDeclared (below), contains an array of 12 int.
   When a functionPointer is used, the functionPointerDeclared is set to 1

   Each 12 pointer to void function are called when a button or joystick state is pressed/reached.
   functionPointer[0] to [7] refers from START_BUTTON to BUTTON_6
   See the TABLE OF COMMUNICATION VALUE/PIN'S (below) to see how it's done
   functionPointer[8] to functionpointer[11] refers from JOYSTICK_TOP to JOYSTICK_RIGHT

   How does it work:
   It is simple, on Master, we can attach a function when a button or joystick state is pressed/reached.
   For example, we imagine we have a game and we want to initialize the game in a function called initGame() when START_BUTTON is pressed.
   For it, just call attachActionToFunc(BUTTON_START, initGame); in the setup() and when the START_BUTTON is pressed, the initGame function is directly called.
   No need for isStartButtonPressed() function on loop for it.

   Code Example:
   void setup() {
    if(initArcadeLib(MASTER_MODE)){
      if(communicationTest()){
        // All is ok
		// Call the function initGame when BUTTON_START is pressed
        attachActionToFunc(BUTTON_START, initGame);
      }else{
       // Problem with communication
      }
    }else{

    }
  }
  void loop(){
   getActualButtonStates();
  }
  void initGame(){
    // Init the game
  }
*/
void (*functionPointer[11])(); /**< Array of functions pointers. Value [0] is for Button Start, [1] for Button Stop, [2] for button 1, [3] for button 2 [7] for button 8,  [9] for joystick position (TOP, LEFT, BOTTOM OR RIGHT), [10] for joystick ..  [12] for 
joystick */
int functionPointerDeclared[11] = {0}; /**< This equal 1 when a functionPointer is declared 0 when not. Look the mapStateButtonJoystickToFunc() function */
int listOfSSToHIGH[6] = {0}; /**<  list of SS pin to put on HIGH state when communicating with Arduino Slave */
/*--------------------------------------------*/

/*
    *SLAVE* mode global variable
	Those variable are used into Slave mode only.
*/

char buttonPinOrder[] = {A0, A1, 4, 5, 6, 7, 8, 9}; /**< buttonPinOrder are the pins set for buttons */
char joystickPinOrder[] = {A2, A3, A4, A5}; /**< joystickPinOrder are the pins set for actions. */

volatile uint8_t lastButtonState   = 0x00, lastJoystickState  = 0x00; /**< lastButtonState and lastJoystickState are volatile variables that contain the last state of button and joystick. They are used into Slave's loop() and this is the value returned when 
Master asks for last button state or last joystick state */

volatile bool isInterruptActivated = false; /**< isInterruptActivated is checked in the loop() of Slave. When the data (0x04) is received on Slave, this means the interruptActivated has to be changed into true value. It is used in volatile because it is shared 
between interrupt (SPI) and loop*/
/*--------------------------------------------*/


/*!
    How is communication structured:
    SPI communication is used between each Arduino (On Slave and One Master)
    SPI is a protocol based on 8 bits communication.
    We use it like this:
    Master sends 8 bits of request to Slave. In our case, these bits can have value of: BUTTON_STATE (00000001)OR(0x01), JOYSTICK_STATE(00000010)OR(0x02) and COMMUNICATION_TEST (00000011)OR(0x03) and so on..
    Slave receive this request from Master, does what is requested and wait the next request of Master to respond:
	
    The next request of Master is a "dummy" byte with 0 on each bits like this: (00000000)OR(0x00) to receive the response of Slave
    For example:
	
    1. Master--(BUTTON_STATES)------>Slave (Slave receive BUTTON_STATE command and the Slave gets the lastButtonState to gets button state)
    2. Master--(0000000)----------->Slave
       Master<--(lastButtonState)-Slave
	   
	1: Master sends a request to the Slave (a request of to have button states = pressed/not pressed buttons) (in this case, the slave doesn't respond)
	2: The Master resends a null byte (00000000) request to the Slave.
	   The Slave respond in the same time (with button states = pressed/not pressed buttons)

    OR (to get joystick states)

    1. Master--(JOYSTICK_STATES)------>Slave (Slave receive JOYSTICK_STATES command and the Slave gets the lastJoystickState to gets joystick state)
    2. Master--(0000000)----------->Slave
       Master<--(lastJoystickState)-Slave
	   
	
	How does Slave respond to BUTTON_STATES OR JOYSTICK_STATES ?:
    For example, the first bit of lastButtonState refers to the value of START_BUTTON. If this value equals 1, the button START_BUTTON is pressed.
    There is 8 button (START_BUTTON, STOP_BUTTON, BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5, BUTTON_6)
    and there is 1 joystick with 4 states (JOYSTCK_TOP, JOYSTCK_LEFT, JOYSTCK_BOTTOM, JOYSTCK_RIGHT)
	So for example, if START_BUTTON is pressed, the lastButtonState will have a value of 10000000(0x80) and if all buttons are pressed it will have a value of 11111111(0xFF)
	For Joystick, it is the same but with only 4 values (JOYSTCK_TOP, JOYSTICK_LEFT, JOYSTICK_BOTTOM, JOYSTICK_RIGHT).
	If the joystick is on the top, the lastJoystickState will have a value of 10000000. If the joystick is on left, lastJoystickState will have a value of 01000000.
	We use only 4 first bits of the variable. The last 4 bits are always 0 (because we have only 4 states for joystick).
	
    It is based on 8 buttons (1 bit per button) in this format:

    TABLE OF COMMUNICATION VALUE/PIN'S:
    FOR the states of button:
    Bit position|Name         | PIN
    0           |START_BUTTON | A0
    1           |STOP_BUTTON  | A1
    2           |BUTTON_1     | 4
    3           |BUTTON_2     | 5
    4           |BUTTON_3     | 6
    5           |BUTTON_4     | 7
    6           |BUTTON_5     | 8
    7           |BUTTON_6     | 9

    FOR the states of joystick:
    Bit position|Name          | PIN
    0           |JOYSTCK_TOP   | A2
    1           |JOYSTCK_LEFT  | A3
    2           |JOYSTCK_BOTTOM| A4
    3           |JOYSTCK_RIGHT | A5
    4           |X (not used)  | X
    5           |X (not used)  | X
    6           |X (not used)  | X
    7           |X (not used)  | X

    What goes with what ?
    1.  SLAVE,
    1.1 BUTTONS:
      uint8_t lastButtonState, getButtonPinStates()
    1.2 JOYSTICK:
      uint8_t lastJoystickState, getJoystickPinStates()
    2. MASTER,
    2.1 BUTTONS:
      uint8_t buttonStatesMaster, getActualButtonStates()
    2.2 JOYSTICK:
      uint8_t joystickStatesMaster, getActualJoystickStates()
*/

/**
 * This function init the library.
 * @brief initArcadeLib
 * @param mode an integrer value that is equals to SLAVE_MODE or MASTER_MODE
 * @return 1 if all is Ok, -1 if error
 */
int initArcadeLib(Mode mode, int SlaveSelectPin) {
    actualMode = mode;

    if (isSlave()) {
        if (!activateSlaveMode())
            return -1;
    } else {
        actualMode = MASTER_MODE;
		
		if(SlaveSelectPin != -1) // If we have set the SlaveSelectPin to a pin number, we set this
			setMasterSSToSlavePin(SlaveSelectPin);

        if (!activateMasterMode())
            return -1;

    }

    return 1;
}


/* --------------------------------------------


   The functions below are for Master Mode only


   ---------------------------------------------
*/

/**
 * This function activate the master mode
 * @brief activateMasterMode
 * @return 1 if ok, -1 if error
 */
int activateMasterMode() {
    /**
      Because Master receive a state change on interrupt pin 2  when Slave's button change,
      we have to clear it (because if we don't, attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), getActualButtonStates, CHANGE)
      could directly enter on Interrupt state/be called because it has an old changing state */
    EIFR = bit (INTF0);  // clear flag for interrupt 0
    EIFR = bit (INTF1);  // clear flag for interrupt 1

    // Dumb security check
    if (!isMaster()) {
        return -1;
    }
	
    /** This pin INTERRUPT_PIN (2) is an interrupt pin (look on http://gammon.com.au/interrupts to have plenty well explained information)
	  Interrupts are used to stop what the Arduino is doing now to do something else when receiving an external signal on D2 or D3 pin (on Nano).
      When there is: FALLING OR RISING OR CHANGE state on this pin, the function on attachInterrupt() 2nd argument is called
      In our case, when Slave's button state change, it sends (LOW OR HIGH) state on Master's pin 2 and when this change, the function callButtonAndJoystickStatesFunctions is called
    */
    pinMode(INTERRUPT_PIN, INPUT); // This input is pulled down with a resistor
	// Attach the interrupt on pin INTERRUPT_PIN (2) to the function callButtonAndJoystickStatesFunctions. So when something changes on this pin (HIGH TO LOW signal OR LOW TO HIGH signal), the funciton is called
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), callButtonAndJoystickStatesFunctions, CHANGE);
	// This set the Slave Select pin as an output (look on  SPI)
    pinMode(SSPin, OUTPUT);
    // Put SS Pin to HIGH (Slave not selected).
    digitalWrite(SSPin, HIGH);

    SPI.begin();
	// Set the communication speed on 128 (16 Mhz/128) --> this is equal to (16'000'000/128=125'000) so 125Khz
    SPI.setClockDivider(SPI_CLOCK_DIV128);

    return 1;
}

/**
 * This function is called when an interrupt occur on Master's 2 pin.
 * It calls getActualButtonStates and getActualJoystickStates functions
 * @brief callButtonAndJoystickStatesFunctions
 */
void callButtonAndJoystickStatesFunctions(){
	getActualButtonStates();
	getActualJoystickStates();
}

/**
 * This function is used when we want to use a different pin than the default pin (10).
 * Sometimes, we just can't use pin 10 for SlaveSelect, so we need to change it (some libraries make pin 10 as a default pin)
 * @brief setMasterSSToSlavePin
 * @param sspin the pin that we want to use for Slave Select
 */
void setMasterSSToSlavePin(int sspin) {
    SSPin = sspin;
}

/**
 * This function is used when Master has to send a command and receive a response from Slave. Request and Response are done in this function
 * @brief sendAndGetData
 * @param command must be: BUTTON_STATE, JOYSTICK_STATE or COMMUNICATION_TEST and so on
 * @param response a variable pointer where the returned response will be stored.
 * @return 1 if all is ok, -1 if error
 */
int sendAndGetData(uint8_t command, uint8_t *response) {
	// We put others SS pin to HIGH
	putOtherSSPinsToHIGH();
    if (!isMaster()) {
        return -1;
    }

    delayMicroseconds(5);
    // Put SSPin to LOW (we want to communicate with Slave)
    digitalWriteFast(SSPin, LOW);
    // Send the command to Slave
    SPI.transfer(command);
    // Wait some time because the Slave must have time to do actions
    delayMicroseconds(150);
    // Put pin to High (We stop to communicate with Slave)
    digitalWriteFast(SSPin, HIGH);
    delayMicroseconds(5);
	// Put SSPin to LOW (we want to communicate with Slave)
    digitalWriteFast(SSPin, LOW);
    delayMicroseconds(5);
	
    // We sent "dummy" bits (00000000) to Slave and receive the response of the last command
    *response = SPI.transfer(0x00);

    digitalWriteFast(SSPin, HIGH);
    delayMicroseconds(5);

    return 1;
}

/**
 * This function return the actual button state on Master.
 * This function call sendAndGetData (that sends a request and receive button state response from Slave).
 * The response is stored into buttonStatesMaster
 * @brief getActualButtonStates
 */
void getActualButtonStates() {
    uint8_t response;
    if (sendAndGetData(BUTTON_STATES, &response)) {
		/* If the activateInterrupt(..) function have been set with REMAIN_HIGH_UNTIL_NEXTCALL argument, it uses OR bit operator. 
		   In this case, when a button is pressed, it will remain HIGH (bit on 1 state) even if the button is no more pressed 
		   It will be set to 0 only when a function like isStartButtonPressed(), isStopButtonPressed(), isButton1Pressed().... is called.*/
		if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL)
			buttonStatesMaster = buttonStatesMaster | response; // OR bit operator
		else
			buttonStatesMaster = response;
			
        mapStateButtonJoystickToFunc();
    } else {
		// Error
    }
}

/**
 * This function return the actual joystick state on Master.
 * This function call sendAndGetData (that sends a request and receive joystick state response from Slave).
 * The response is stored into joystickStatesMaster
 * @brief getActualJoystickStates
 */
void getActualJoystickStates() {
    uint8_t response;
    if (sendAndGetData(JOYSTICK_STATES, &response)) {
		/* If the activateInterrupt(..) function have been set with REMAIN_HIGH_UNTIL_NEXTCALL argument, it uses OR bit operator. 
		   In this case, when a joystick is pressed, it will remain HIGH (bit on 1 state) even if the button is no more pressed 
		   It will be set to 0 only when a function like isStartButtonPressed(), isStopButtonPressed(), isButton1Pressed().... is called.*/
		if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL)
			joystickStatesMaster = joystickStatesMaster | response; // OR bit operator
		else
			joystickStatesMaster = response;
			
        mapStateButtonJoystickToFunc();
    } else {
		//Error
    }
}

/**
 * This function should be used in setup() before trying to do any request. It ensures that the communication between Slave and Master is ok.
 * It can be used on loop to ensure the communication is always ok, but this takes time (175microseconds).
 * It send a bit sequence (00000011)OR(0x03) and wait a response from Slave: 0xAA (10101010)
 * If the value returned is 10101010, the communication worked, in other case, it didn't.
 * @brief communicationTest
 * @return true if communication worked, false if not
 */
bool communicationTest() {
    uint8_t response;
    if (sendAndGetData(COMMUNICATION_TEST, &response)) {
        if (response == 0xAA)
            return true;
        else
            return false;
    }

    return false;
}

/**
 * This function is called to activate interrupt on Slave (from the Master)
 * There is 2 mode for interrupts: EXTERNAL_PIN_INTERRUPT and TIMER_INTERRUPT
 * For EXTERNAL_PIN_INTERRUPT, the Master sends to Slave a byte with the value of 0x04 (00001000). See activateExternalPinInterrupt() function for more informations
 * For TIMER_INTERRUPT, the Master sends to the Slave a request each 10ms to ask Slave the actual button states
 * By default, EXTERNAL_PIN_INTERRUPT is activated
 * @brief activateInterrupt
 */
void activateInterrupt(Interrupt interrupt, InterruptMode interrupt_mode){

	switch(interrupt){
		case EXTERNAL_PIN_INTERRUPT:
			actualInterruptMode = interrupt_mode;
			activateExternalPinInterrupt();
		break;
		case TIMER_INTERRUPT:
			activateTimerInterrupt();
		break;
	}
	delayMicroseconds(5);
}

/**
 * This function is called in activateInterrupt function (in the switch)
 * It send 0x04 command to Slave to activate/desactivate interrupt. 
 * If interrupt was desactivated on Slave, it turns it ON, it it was activated, it turns it OFF.
 * @brief activateExternalPinInterrupt
 */
void activateExternalPinInterrupt(){
	uint8_t response;
		if (sendAndGetData(INTERRUPT_TURNONOFF, &response)) {
			if(response == 0x01){
				// Interrupt was desactivated on Slave, it is now ACTIVATED
			}else if(response == 0x00){
				// Interrupt was activated on Slave, it is now DESACTIVATED
				activateExternalPinInterrupt(); // We want to activate interrupt so we resend the same command because it is now desactivated. Afer this call, it will be activated
			}else{
				// -------[ERROR]-------: activateInterrupt() should return 0x01 or 0x00, but it has returned something else
			}
			
		} else {
			// Error
		}
}

/**
 * This function is called in activateInterrupt function (in the switch)
 * It stops what the Arduino is doing right now and sends a request to the Slave the actual states of button and joystick
 * To understand more about this, look on http://www.instructables.com/id/Arduino-Timer-Interrupts/
 * @brief activateTimerInterrupt
 */
void activateTimerInterrupt(){
  //set timer1 interrupt at 100Hz (timer each 100 ms)
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 100hz increments
  OCR1A = 1562; // = (16*10^6) / (10*1024) - 1 (must be <65536). 100ms
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
}

/**
 * This function is called every 100 ms (accordoing to activatetimerInterrupt() function whera all is initialized)
 * When activated, this timer call callButtonAndJoystickStatesFunctions() every 100 ms. This is an interrupt, it means Arduino stop was he was doing and call thoses functions
 *
 *
 */
ISR(TIMER1_COMPA_vect){
  callButtonAndJoystickStatesFunctions();
}

/**
 * This library use SPI communication with the Arduino Slave. But there is some other stuff (like TFT screen) that uses SPI communication too.
 * Regarding to SPI protocol communication, we have to put LOW the SlaveSelect (SS) pin when we want to speak with a Slave and put it HIGH when we don't.
 * When we have multiple Slave, we must put to LOW all SS pin to HIGH except the one we want to speak to
 *
 * For example, if we have a TFT screen with SS pin of this one attached to pin 9, and the Arduino Slave attached to pin 10:
 * When we want to speak to TFT: Put LOW pin 9 and HIGH pin 10
 * When we want to speak to Arduino Slave: Put LOW pin 10 and HIGH pin 9
 * 
 * This function get as parameter a list of SS pin to put to HIGH when we speak to Arduino Slave
 * This function must be called in the setup()
 * 
 * @brief addSSPInToDesactivate
 */
void addSSPInToDesactivate(int listToPutOnHigh[]){
	
	for(unsigned int i = 0; i < sizeof listToPutOnHigh; i++){
		listOfSSToHIGH[i] = listToPutOnHigh[i];
	}
}

/**
 * 
 * Called into the sendAndGetData function to put all others pins (other than the SS pin going from Arduino Master to Arduino Slave) on HIGH state
 * @brief putOtherSSPinsToHIGH
 */
void putOtherSSPinsToHIGH(){
	for(unsigned int i = 0; i < 6; i++){
		if(listOfSSToHIGH[i] != 0){
			digitalWriteFast(listOfSSToHIGH[i], HIGH);
		}
	}
}

/**
 * Return if the actualMode is Master
 * This is only used to avoid using Master functions on Slave Arduino OR Slave functions on Master Arduino
 * @brief isMaster
 * @return true if all is ok, false if we use a master function in Slave Mode.
 */
bool isMaster() {
    return (actualMode == MASTER_MODE) ?  true :  false;
}

/**
 * Return if the actualMode is Slave
 * This is only used to avoid using Slave functions on Master Arduino OR Master functions on Slave Arduino
 * @brief isSlave
 * @return ture if all is ok, false if we use a slave function in Master Mode.
 */
bool isSlave() {
    return (actualMode == SLAVE_MODE) ?  true :  false;
}

/**
 * Check if the button or joystick passed into argument (button_joystick) is pressed or not
 * @brief isButtonOrJoystickPressed
 * @return ture if all is ok, false if we use a slave function in Master Mode.
 */
bool isButtonOrJoystickPressed(Button_Joystck button_joystick){
	/* 	This is a bit tricky. On the enum Button_Joystck, there is a list of all buttons and joystick states: from START_BUTTON, STOP_BUTTON, BUTTON_1 ....  to JOYSTCK_BOTTOM, JOYSTCK_RIGHT
		The first enum value START_BUTTON equals to 0, the last JOYSTCK_RIGHT equals to 11
		All states sent to Master (from Slave) are on 8 bits: The first bit is START_BUTTON, the second is STOP_BUTTON and so on (look on the top of this file and read the paragraphes beginning with "How is communication structured" if not done yet):
		When we want to know if a button has been pressed. We shift the bits to the right and make a AND bit operator on it.
		For example we say that the buttonStatesMaster equals to 10000000
		To know if the START_BUTTON has been pressed, we shift the bits to the right by the 7 - value of enum (here 7 - START_BUTTON ---> = 7 - 0 = 7).
		In this case we shift all bits to the right by 7 so the first bit will be at the end. Like this: 00000001
		Then we do a AND operator on it: & (0x01). So if the last bit is equals to 1, this will be equals to 1 (true), otherwise it will be 0 (false) 
		
		An other example buttonStatesMaster will equal to 00100000
		We want to know if BUTTON_1 is pressed. The enum value of BUTTON_1 equals to 2. So we shift to the right 7 - value of enum (here 7 - BUTTON_1 ---> = 7 - 2 = 5)
		In this case, the bits will be shifted to the right by 5. The third bit will be at the end so it will equal 00000001. Then we do a AND operator on it like before..
		
		But there is joystick states too on the enum. Those begin from JOYSTCK_TOP to JOYSTCK_RIGHT with enum values from 8 to 11.
		In this case, we just make it in a different "way" but the result will be the same (true or false)
		For example to check if JOYSTCK_TOP is pressed, we shift the bits to the right by 11 - button_joystick + 4 (JOYSTCK_TOP is equal to 8).
		So, it will in all cases be shifted to the last bit position and a AND bit operator will be applied (0x01)
	*/
	
	if(button_joystick >= JOYSTCK_TOP)
		return (joystickStatesMaster >> (11-button_joystick+4)) & (0x01);
	else
		return (buttonStatesMaster >> (7-button_joystick)) & (0x01);
}

/**
 * Return true if the START_BUTTON has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isStartButtonPressed
 * @return true if START_BUTTON has been pressed
 */
bool isStartButtonPressed() {
	bool toReturn = isButtonOrJoystickPressed(START_BUTTON);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 7) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x80;// XOR binary operator. In this case, put the first bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if STOP_BUTTON has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isStopButtonPressed
 * @return true if STOP_BUTTON has been pressed
 */
bool isStopButtonPressed() {
	bool toReturn = isButtonOrJoystickPressed(STOP_BUTTON);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 6) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x40;// XOR binary operator. In this case, put the second bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if BUTTON_1 has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isButton1Pressed
 * @return true if BUTTON_1 has been pressed
 */
bool isButton1Pressed() {
	bool toReturn = isButtonOrJoystickPressed(BUTTON_1);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 5) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x20;// XOR binary operator. In this case, put the third bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if BUTTON_2 has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isButton2Pressed
 * @return true if BUTTON_2 has been pressed
 */
bool isButton2Pressed() {
	bool toReturn = isButtonOrJoystickPressed(BUTTON_2);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 4) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x10;// XOR binary operator. In this case, put the forth bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if BUTTON_3 has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isButton3Pressed
 * @return true if BUTTON_3 has been pressed
 */
bool isButton3Pressed() {
	bool toReturn = isButtonOrJoystickPressed(BUTTON_3);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 3) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x08;// XOR binary operator. In this case, put the fifth bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if BUTTON_4 has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isButton4Pressed
 * @return true if BUTTON_4 has been pressed
 */
bool isButton4Pressed() {
	bool toReturn = isButtonOrJoystickPressed(BUTTON_4);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 2) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x04;// XOR binary operator. In this case, put the sixth bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if BUTTON_5 has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isButton5Pressed
 * @return true if BUTTON_5 has been pressed
 */
bool isButton5Pressed() {
	bool toReturn = isButtonOrJoystickPressed(BUTTON_5);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 1) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x02;// XOR binary operator. In this case, put the seventh bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if BUTTON_6 has been pressed
 * This is based on the value returned on getActualButtonStates function.
 * @brief isButton6Pressed
 * @return true if BUTTON_6 has been pressed
 */
bool isButton6Pressed() {
	bool toReturn = isButtonOrJoystickPressed(BUTTON_6);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (buttonStatesMaster >> 0) & (0x01))
			buttonStatesMaster = buttonStatesMaster ^ 0x01;// XOR binary operator. In this case, put the eighth bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if the joystick is on Top
 * This is based on the value returned on getActualJoystickStates function.
 * @brief isJoystickTop
 * @return true if JOYSTCK_TOP has been reached
 */
bool isJoystickTop() {
	bool toReturn = isButtonOrJoystickPressed(JOYSTCK_TOP);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (joystickStatesMaster >> 7) & (0x01))
			joystickStatesMaster = joystickStatesMaster ^ 0x80;// XOR binary operator. In this case, put the first bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if the joystick is on Left
 * This is based on the value returned on getActualJoystickStates function.
 * @brief isJoystickLeft
 * @return true if JOYSTCK_LEFT has been reached
 */
bool isJoystickLeft() {
	bool toReturn = isButtonOrJoystickPressed(JOYSTCK_LEFT);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (joystickStatesMaster >> 6) & (0x01))
			joystickStatesMaster = joystickStatesMaster ^ 0x40;// XOR binary operator. In this case, put the second bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if the joystick is on Bottom
 * This is based on the value returned on getActualJoystickStates function.
 * @brief isJoystickBottom
 * @return true if JOYSTCK_BOTTOM has been reached
 */
bool isJoystickBottom() {
	bool toReturn = isButtonOrJoystickPressed(JOYSTCK_BOTTOM);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (joystickStatesMaster >> 5) & (0x01))
			joystickStatesMaster = joystickStatesMaster ^ 0x20;// XOR binary operator. In this case, put the third bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * Return true if the joystick is on Right
 * This is based on the value returned on getActualJoystickStates function.
 * @brief isJoystickRight
 * @return true if JOYSTCK_RIGHT has been reached
 */
bool isJoystickRight() {
	bool toReturn = isButtonOrJoystickPressed(JOYSTCK_RIGHT);
	
	if(actualInterruptMode == REMAIN_HIGH_UNTIL_NEXTCALL){
		if(toReturn) // If to return is true (or if the bit equals to 1 from (joystickStatesMaster >> 4) & (0x01))
			joystickStatesMaster = joystickStatesMaster ^ 0x10;// XOR binary operator. In this case, put the forth bit to 0 in the case it was on 1. Only used in REMAIN_HIGH_UNTIL_NEXTCALL mode for activateInterrupt(EXTERNAL_INTERRUPT, REMAIN_HIGH_UNTIL_NEXTCALL)
	}
    return toReturn;
}

/**
 * This function can attach a button or joystick state to a function.
 * For example, we can attach the START_BUTTON to a function called initGame (like on example begining at line 46).
 * When called, the index of functionPointerDeclared[button_joystick] is also put to 1.
 * @brief attachActionToFunc
 * @param button_joystick
 */
void attachActionToFunc(Button_Joystck button_joystick, void(*f)()) {
    functionPointer[button_joystick] = f;
    functionPointerDeclared[button_joystick] = 1;
}

/**
 * This function check if a pointer function has been declared for one button State.
 * For example, when you want that a button state (like START_BUTTON) call a function when it is pressed, you use attachActionToFunc(START_BUTTON, theFunctionWhereToGo)
 * @brief isPointerFunctionDeclared
 * @param button_joystick
 * @return true if there is one attachActionToFunc on the button_joystick, false if not
 */
bool isPointerFunctionDeclared(Button_Joystck button_joystick) {
    if (functionPointerDeclared[button_joystick] == 1)
        return true;
    else
        return false;
}

/**
 * This function is called into getActualButtonStates and getActualJoystickStates.
 * In those 2 functions, we call sendAndGetData that returns back the response of Slave.
 * Then if we have already set a pointer to a function with attachActionToFunc( START_BUTTON OR STOP_BUTTON OR BUTTON_1 ..., functionToGo), this function call the function in second parameter
 * @brief mapStateButtonJoystickToFunc
 */
void mapStateButtonJoystickToFunc() {
    if (isButtonOrJoystickPressed(START_BUTTON)) {
        if (isPointerFunctionDeclared(START_BUTTON)) {
            (*functionPointer[0])();
        }
    }
    if (isButtonOrJoystickPressed(STOP_BUTTON)) {
        if (isPointerFunctionDeclared(STOP_BUTTON)) {
            (*functionPointer[1])();
        }
    }
    if (isButtonOrJoystickPressed(BUTTON_1)) {
        if (isPointerFunctionDeclared(BUTTON_1)) {
            (*functionPointer[2])();
        }
    }
    if (isButtonOrJoystickPressed(BUTTON_2)) {
        if (isPointerFunctionDeclared(BUTTON_2)) {
            (*functionPointer[3])();
        }
    }
    if (isButtonOrJoystickPressed(BUTTON_3)) {
        if (isPointerFunctionDeclared(BUTTON_3)) {
            (*functionPointer[4])();
        }
    }
    if (isButtonOrJoystickPressed(BUTTON_4)) {
        if (isPointerFunctionDeclared(BUTTON_4)) {
            (*functionPointer[5])();
        }
    }
    if (isButtonOrJoystickPressed(BUTTON_5)) {
        if (isPointerFunctionDeclared(BUTTON_5)) {
            (*functionPointer[6])();
        }
    }
    if (isButtonOrJoystickPressed(BUTTON_6)) {
        if (isPointerFunctionDeclared(BUTTON_6)) {
            (*functionPointer[7])();
        }
    }
    if (isButtonOrJoystickPressed(JOYSTCK_TOP)) {
        if (isPointerFunctionDeclared(JOYSTCK_TOP)) {
            (*functionPointer[8])();
        }
    }
    if (isButtonOrJoystickPressed(JOYSTCK_LEFT)) {
        if (isPointerFunctionDeclared(JOYSTCK_LEFT)) {
            (*functionPointer[9])();
        }
    }
    if (isButtonOrJoystickPressed(JOYSTCK_BOTTOM)) {
        if (isPointerFunctionDeclared(JOYSTCK_BOTTOM)) {
            (*functionPointer[10])();
        }
    }
    if (isButtonOrJoystickPressed(JOYSTCK_RIGHT)) {
        if (isPointerFunctionDeclared(JOYSTCK_RIGHT)) {
            (*functionPointer[11])();
        }
    }
}

/* --------------------------------------------


   The functions below are for Slave Mode only


   ---------------------------------------------
*/

/**
 * This function activate the slave mode
 * @brief activateSlaveMode
 * @return 1 if ok, -1 if error
 */
int activateSlaveMode() {

    // Turn on SPI in slave mode
    SPCR |= _BV(SPE);

    // Turn on interrupts
    SPCR |= _BV(SPIE);

    pinMode(INTERRUPT_PIN, OUTPUT);
    pinMode(MISO, OUTPUT);
    pinMode(10, INPUT);

	/*// All pins are put in pullup mode
	for (unsigned int i = 0; i < sizeof buttonPinOrder; i ++) {
		pinMode(buttonPinOrder[i], INPUT_PULLUP);
	}
	for (unsigned int i = 0; i < sizeof joystickPinOrder; i ++) {
		pinMode(joystickPinOrder[i], INPUT_PULLUP);
	}*/

    return 1;
}

/**
  * This is an interrupt for SPI communication (on Slave). Have a look on http://www.gammon.com.au/forum/?id=10892 and https://www.arduino.cc/en/Tutorial/SPIEEPROM to understand how general concept works
  * If so, this function do these things:
  * - Get data came on SPI Data Register variable (SPDR)
  * - Look on this variable and make an action
  * - Respond
  * @brief ISR (SPI_STC_vect)
  *
  */
ISR (SPI_STC_vect)
{
    // We receive the command sent by Master and set it to uint8_t command variable
    uint8_t command = SPDR;
    uint8_t resp = 0x00;

	// We look on which kind of command the Master send us
    switch (command) {
    case BUTTON_STATES:
        // We send the response to Master
        resp = lastButtonState;
        SPDR = resp;
        break;
    case JOYSTICK_STATES:
        // We send the response to Master
        resp = lastJoystickState;
        SPDR = resp;
        break;
    case COMMUNICATION_TEST:
        // We send the response to master
        SPDR = 0xAA; // This is equal to b10101010
        break;
	case INTERRUPT_TURNONOFF:
		// We turn on or off interrupt (if interrupt was already on, it is turned off, if it was off, it is turned on
		if(!isInterruptActivated){
			isInterruptActivated = true;
			SPDR = 0x01; // We sent back to the Master tha value 0x01 to tell that the interrupt has been activated 
		}else{
			isInterruptActivated = false;
			SPDR = 0x00; // We sent back to the Master the value 0x00 to tell that the interrupt has been desactivated
		}
		break;
    case 0x00: 
        // Master sends "dummy" bits (00000000). When Slave receive this, nothing is done (because SPDR was already set).
		break;
    default:
        SPDR = 0x00;
		break;
    }
}

/**
 * Function that read actual pins' state on Slave
 * @brief getButtonPinStates
 * @return uint8_t variable value with button state
 */
uint8_t getButtonPinStates() {
    uint8_t buttonStates = lastButtonState;

    if (millis() - lastTimeButton >= 10) {
        buttonStates = 0x00;

        for (unsigned int i = 0; i < sizeof buttonPinOrder; i++) {
            uint8_t button = digitalReadFast(buttonPinOrder[i]);

            buttonStates <<= 1;
            buttonStates |= button;
        }
        lastTimeButton = millis();
    }

    return buttonStates;
}

/**
 * Function that read actual pins' state and return it on 8 bits
 * @brief getJoystickPinStates
 * @return
 */
uint8_t getJoystickPinStates() {
    uint8_t joystickStates = lastJoystickState;

    if (millis() - lastTimeJoystick >= 10) {
        joystickStates = 0x00;

        for (unsigned int i = 0; i < sizeof joystickPinOrder; i++) {
            uint8_t joystick = digitalReadFast(joystickPinOrder[i]);

            joystickStates <<= 1;
            joystickStates |= joystick;
        }
        // Because joystickStates is only 4 values (TOP, LEFT, BOTTOM, RIGHT), we put thoses values in the beginning of the joystickStates with <<= 4
        // Before this action, uint8_t joystickStates looks like this: 0 0 0 0 TOP LEFT BOTTOM RIGHT
        // After it, the bits are more like this: TOP LEFT BOTTOM RIGHT 0 0 0 0
        // Where TOP LEFT BOTTOM RIGHT are the state of joystick (1 if "pressed" 0 if not)
        joystickStates <<= 4;
        lastTimeJoystick = millis();
    }

    return joystickStates;
}

