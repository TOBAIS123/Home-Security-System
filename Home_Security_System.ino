/*
	Erfan Y, Nicholas G, Kevin N
	Mr. Wong
    April 19th, 2021
    This program simulates a home security system that has many sensors
    and tools to alert and aid against home invasions. 
    	A LDR is used to turn the door lamp (white LED) on, while the potentiometer is used to control its brightness.
        A DIP switch simulates a control panel with different states. These states are then checked with sensors to see if any actions should be done.
        The door sensor tells whether the door is open or not, the force sensor measures an external force on entry points, 
        and a ultrasonic distance sensor measures if objects are within a certain distance within the house.
        All statuses are displayed on an LCD which correlates with any actions done.
        An IR remote and sensor override the control panels password function, while a red LED displays the status of the force and distance sensors.
*/

#include <LiquidCrystal.h>
#include <IRremote.h>
#include <stdio.h>

int state = 0; // Determines what actions happen during the security systems runtime

static unsigned long lastBuzzer = 0; // The time in milliseconds since the last buzzer happened 

static unsigned long lastReset = 0; // The time in milliseconds since the last reset message happened

static unsigned long lastBlink = 0; // The time in milliseconds since the last blink (of the LED) happened

unsigned long blinkStartTime; // The initial time of the program, used to track the timing of the LED blinks

bool reset; // A boolean determining when the reset statment should start and end

bool alarmDone = false; // A boolean that makes sure the alarm does not go off when resetting the DIP switches

bool remotePassword; // Represents whether the correct password was entered

bool isBlinking = false; // Represents a state where the LED should blink

bool forceSensor; //Represents if a force is strong enough to set off the alarm

bool distanceSensor; //Represents if an object is close enough to set off the alarm

bool password[2] {0, 1}; // The array representing the password in binary

LiquidCrystal lcd(2, 3, 4, 5, 6, 7); // Initializes the LCD screen to the pins on the Arduino

IRrecv irrecv(16); // IR sensor signal pin at 16 (A2)

decode_results results; // Input from IR remote

// Returns true if the door is closed
bool doorClosed() {
  return digitalRead(9);
}

// Returns the state which the DIP switches represent
int getAlarmSetting() {
  bool dip1 = digitalRead(13);
  bool dip2 = digitalRead(12);
  if (!dip1 && dip2) { // Away
    return 1;
  } else if (dip1 && !dip2) { // Stay home
    return 2;
  } else { // Off
    return 0;
  }
}

// Returns true if the password entered by the DIP switch or IR remote is correct
bool verifyPassword() {
  if (remotePassword) { // Verifies if the IR remote has entered the password
    return true;
  }
  bool dip3 = digitalRead(11);
  bool dip4 = digitalRead(10);
  if (dip3 == password[0] && dip4 == password[1]) { // Verifies whether the DIP switch password has been entered
    return true;
  }
  return false;
}

//Returns true if an object is detected within the range of 100-200m using an Ultrasonic Distance Sensor
bool motionDetected() {
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW); // Set to low to send clean ping
  delayMicroseconds(2);
  digitalWrite(18, HIGH); // Send ping
  delayMicroseconds(10);
  digitalWrite(18, LOW);  // Set to low to send clean ping
  pinMode(18, INPUT);
  int distance = pulseIn(18, HIGH) / 33; // Receive ping and find distance
  if (distance > 100 && distance < 200) { //If it is between the distance, the red LED indicator starts to blink
    return true;
  }
  return false;
}

// Returns true if there is an external force of 7N or above based on the Force Sensor
bool forceDetected() { 
  if (analogRead(A3) > 782) { // if above 7N, the red LED indicator starts to blink
    return true;
  }
  return false; 
}

// Returns true if the LDR detects above a certain threshold of brightness
bool lightDetected() {
  if (analogRead(A0) < 480) { // if LDR bar is below halfway approximately
    return false;
  }
  return true;
}

// Beeps the buzzer periodically for an interval of time
void buzzerBeep (int time){
  if (lastBuzzer == 0) {
    lastBuzzer = millis(); // sets time when buzzer started
  }
  
  if (millis() - lastBuzzer < time) { // if buzzer hasn't exceeded time interval it beeps
    digitalWrite(15,HIGH);
    delay(200);
    digitalWrite(15,LOW);
    delay(200);
  } else { // time interval has elapsed, buzzer stops beeping
    digitalWrite(15,LOW);
    lastBuzzer = 0;
  }
}

// Chimes the buzzer for an interval of time
void buzzerChime(int time){
  if(lastBuzzer == 0) {
    lastBuzzer = millis(); // sets time when chime started
  }
  
  if(millis() - lastBuzzer < time) { // if chime hasn't exceeded time interval it plays a tone
    digitalWrite(15,HIGH);
  } else { // time interval has elapsed
    digitalWrite(15,LOW);
    lastBuzzer = 0;
    reset = true;
  }
}

// Blinks the red LED with intervals of 200 milliseconds
void ledBlink() {
  if (millis() - blinkStartTime >= 200 && isBlinking)  
  {
    digitalWrite(19, !digitalRead(19));  
    blinkStartTime = blinkStartTime + 200;  
  }
}

// Stops LED blinking (stays on)
void ledStop() {
  digitalWrite(19, HIGH);
  isBlinking = false;
}

// Prints two line message on the LCD screen
void lcdPrintMessage(char line1[], char line2[]) {
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0, 1); // sets cursor to next line
  lcd.print(line2);
}

// Prints given setting on the LCD screen
void lcdPrintSetting(char setting[]) {
  lcd.clear();
  lcd.print("Alarm Setting:");
  lcd.setCursor(0, 1); // sets cursor to next line
  lcd.print(setting);
}

// Sets all pinmodes, starts necessary services
void setup() {
  pinMode(A0, INPUT);
  pinMode(A3, INPUT);
  pinMode(8, OUTPUT);
  pinMode(9, INPUT);
  pinMode(10, INPUT);
  pinMode(11, INPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  pinMode(15, OUTPUT);
  pinMode(19, OUTPUT);
  
  digitalWrite(19, HIGH); // Turn on red LED
  
  irrecv.enableIRIn(); // Start the infrared receiver
  
  Serial.begin(9600); // Starts console (for debugging)

  blinkStartTime = millis(); //initial time to set up blink statemachine
  
  lcd.begin(16, 2); // Starts LCD screen with given size
  lcdPrintSetting("Off"); // Shows initial alarm setting
}
 
// Checks and changes states, monitors numerous components
void loop() {

  // Calls sensor functions to set the boolean values of the respective variables
  forceSensor = forceDetected();
  distanceSensor = motionDetected();
    
  // If either the force sensor or ultrasonic distance sensor detect an object or force within their range,
  // a red LED starts to blink with a period of 200 milliseconds 
  if (forceSensor || distanceSensor){
    isBlinking = true;
	ledBlink(); 
  } else{
      ledStop(); // stops red LED blinking
  }
  
  // Controls whether the Door Lamp is on by checking the LDR value 
  if (!lightDetected()) {
    digitalWrite(8, LOW); // Turn on white LED
  } else {
    digitalWrite(8, HIGH); // Turn off white LED
  }
   
  // IR Sensor
  if (irrecv.decode(&results)) { // True if IR remote enters password "1"
    if (results.value == 16582903) { // If "1" button is pressed corresponding to the binary 01
      remotePassword = true;
    }
 	irrecv.resume(); // allows more inputs to be received
  }

  // Makes sure that the alarm does not go off
  if (alarmDone && doorClosed()) {
    alarmDone = false;
  }
  
  int a = getAlarmSetting(); // stores alarm setting for the loop iteration
  // State reset to default if these conditions are met
  if (a == 0 && state != 0 && state != 3 && state != 6 && !reset && !alarmDone) {
    state = 0;
    lcdPrintSetting("Off");
  }

  // Manages what every state does and how the states transition between one another
  switch (state) {
    case 0: // default state, alarm setting off with door closed
      remotePassword = false;
      if (a == 1) { // if alarm setting changed to "away"
        state = 1;
        lcdPrintSetting("Away");
      } else if (a == 2) { // if alarm setting changed to "stay home"
        state = 2;
        lcdPrintSetting("Stay home");
      } else if (!doorClosed() && !alarmDone) {  // if door is opened
        state = 6;
        lcdPrintMessage("WELCOME HOME", "");
      }
    break;
    case 1: // alarm setting "away" with no security threats
      if (a == 2) {  // if alarm setting changed to "stay home"
        state = 2;
        lcdPrintSetting("Stay home");
      } else if (!doorClosed()) {  // if door is opened
        state = 3;
        lcdPrintMessage("ALARM  ACTIVATED", "Enter password");
      }
    	else if (distanceSensor) { // if motion sensor detects object in range
        state = 3;
        lcdPrintMessage("MOTION DETECTED", "Enter password");
      }
       else if (forceSensor && !reset) { // if force sensor detects high force
        state = 3;
        lcdPrintMessage("ENTRY DETECTED", "Enter password");
      }
    break;
    case 2: // alarm setting "stay home" with no security threats
      if (a == 1) { // if alarm setting changed to "away"
        state = 1;
        lcdPrintSetting("Away");
      } else if (!doorClosed()) { // if door is opened
        state = 3;
        lcdPrintMessage("ALARM  ACTIVATED", "Enter password");
      }
    break;
    case 3: // alarm triggered, password must be entered within 5 seconds
      buzzerBeep(5000L); // beeps for 5 seconds
      ledBlink(); // blinks red LED
      if (lastBuzzer == 0) {
        if (verifyPassword()) { // if correct password has been entered (IR remote or DIP switch)
          state = 4;
          lcdPrintMessage("Alarm turned off", "Restart required");
          alarmDone = true;
        } else { // if correct password was not entered
          state = 5;
          lcdPrintMessage("The authorities", "are on their way");
        }
      }
    break;
    case 4: // alarm deactivated by entering password, must be restarted to default state
    reset = false;
    break;
    case 5: // password was not entered during alarm, displays message about authorities being contacted
      buzzerBeep(6000L); // beeps for 6 seconds
      if (lastBuzzer == 0 && !reset) { // if buzzer ends
        state = 4; 
        lcdPrintMessage("Alarm turned off", "Restart required");
        alarmDone = true;
      }
    break;
    case 6: // welcome chime if alarm is off and door was opened
        buzzerChime(1000L); // plays chime for one second
        if (lastBuzzer == 0 || doorClosed()) { // if chime is finished or if door is closed early
          digitalWrite(15, LOW);
          state = 7;
          lcd.clear();
          alarmDone = false;
      }
    break;
    case 7: // waits for restart after welcome chime
      buzzerChime(0);
      if (!doorClosed() && lastReset == 0) { // sets initial time so elapsed time can be measured
        lastReset = millis();
      }
      if (!doorClosed() && reset) { // if door is not closed, prompts user to close door
        lcd.setCursor(0, 0);
        lcd.print("Please close the");
        lcd.setCursor(0, 1);
        lcd.print("door and restart");
        if (millis() - lastReset < 2000L) {
        }
      } else { // after 2 seconds have passed, allows status to reset
        lastReset = 0;
        reset = false;
      }
    break;
  }    
}