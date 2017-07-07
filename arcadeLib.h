#include <SPI.h>
#include "digitalWriteFast.h" // This is only used to write fastly on pins (no use of digitalWrite or digitalRead but direct write to pins)

typedef enum {START_BUTTON=0x00, STOP_BUTTON, BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5, BUTTON_6, JOYSTCK_TOP, JOYSTCK_LEFT, JOYSTCK_BOTTOM, JOYSTCK_RIGHT} Button_Joystck;
typedef enum {SLAVE_MODE, MASTER_MODE} Mode;
typedef enum {EXTERNAL_PIN_INTERRUPT, TIMER_INTERRUPT} Interrupt; 
typedef enum {NORMAL_INTERRUPT_MODE, REMAIN_HIGH_UNTIL_NEXTCALL} InterruptMode;
//typedef enum {BUTTON_STATES = 0x01, JOYSTICK_STATES = 0x02, COMMUNICATION_TEST = 0x03, INTERRUPT_TURNONOFF = 0x04} Command;


#define MOSI 11
#define MISO 12
#define SCK  13

#define INTERRUPT_PIN 2

#define BUTTON_STATES 0x01
#define JOYSTICK_STATES 0x02
#define COMMUNICATION_TEST 0x03
#define INTERRUPT_TURNONOFF 0x04

int initArcadeLib(Mode mode=SLAVE_MODE, int SlaveSelectPin=-1);
bool isMaster();
bool isSlave();
/* --------------------------------------------
 * 
 * 
 * The functions below are for Master Mode only
 * 
 * 
 * ---------------------------------------------
 */
int  activateMasterMode();
void setMasterSSToSlavePin(int sspin);
int  sendAndGetData(uint8_t command, uint8_t *response);
void getActualButtonStates();
void getActualJoystickStates();
bool communicationTest();
bool isStartButtonPressed();
bool isStopButtonPressed();
bool isButton1Pressed();
bool isButton2Pressed();
bool isButton3Pressed();
bool isButton4Pressed();
bool isButton5Pressed();
bool isButton6Pressed();
bool isJoystickTop();
bool isJoystickLeft();
bool isJoystickBottom();
bool isJoystickRight();
void attachActionToFunc(Button_Joystck button_joystick, void(*f)());
void mapStateButtonJoystickToFunc();
void callButtonAndJoystickStatesFunctions();
void activateInterrupt(Interrupt interrupt=EXTERNAL_PIN_INTERRUPT, InterruptMode interrupt_mode=NORMAL_INTERRUPT_MODE);
void activateExternalPinInterrupt();
void activateTimerInterrupt();
void addSSPInToDesactivate(int listToPutOnHigh[]);
void putOtherSSPinsToHIGH();
bool isButtonOrJoystickPressed(Button_Joystck button_joystick);

/* --------------------------------------------
 * 
 * 
 * The functions below are for Slave Mode only
 * 
 * 
 * ---------------------------------------------
 */
int activateSlaveMode();
uint8_t getButtonPinStates();
uint8_t getJoystickPinStates();
