//Defines the pins used on the board

#define dir 3 //Direction Control - D3
#define MS1 4 //One of the pins used to set the step size - D4
#define MS2 5 //The other pin used to set the step size - D5
#define EN  6 //Enables or disables the stepper motor - D6
#define btn 8 //On/Off button input - D8
#define swt 10 //Proximity Sensor Input - D10
#define gled 12 //Green LED Indicator - D12
#define bled 13 //Blue LED Indicator - D13

int stp = 9;  //Speed Control for the Stepper Motor - D9

#include <EEPROM.h>

int pot = A0;
int potValue = 0;
int buttonState = 0;
int switchState = 0;
byte maxSec;
byte maxMil; 
int direct = 0;
int dur = 0;
int x;
int storedButtonState = 0;
int storedPotState = 0;
int swtCount = 0;
float turnTime = 0;
int calTime = 0;

int addr = 0;
int addr1 = 1;

void setup() {

//set-up the digital pins as inputs or outputs
  pinMode(stp, OUTPUT);
  pinMode(dir, OUTPUT);
  pinMode(MS1, OUTPUT);
  pinMode(MS2, OUTPUT);
  pinMode(EN, OUTPUT);
  pinMode(btn, INPUT);
  pinMode(swt, INPUT);
  pinMode(gled, OUTPUT);
  pinMode(bled, OUTPUT);

  //Open serial port for diagnostics
  Serial.begin(9600); //Open Serial connection for debugging
  Serial.println("Begin motor control");
  Serial.println();

  //Set Stepper Drive to full step mode, see the Sparkfun EasyDriver instruction manual for information on other modes
  digitalWrite(MS1,LOW);
  digitalWrite(MS2,LOW);

  readEEPROM(); //calls a subroutine to read the calibration data from EEPROM

  Home(); //calls the home subroutine to make sure the flow gate is closed at start-up

  CalCheck();  //call a subroutine to see if the conditions are present to calibrate the flow gate

  //Set initial values for stored button and potentiometer states
  storedButtonState = digitalRead(btn);
  storedPotState = analogRead(pot);
}

void loop() {

  digitalWrite(gled,LOW); //turn the green led on to indicate normal operation
  digitalWrite(bled,HIGH); //turn the blue led off
  
  potValue = analogRead(pot);  //read the potentiometer value

  buttonState = digitalRead(btn); //read the on/off button state

  //check to see if the button state has changed
  if (buttonState != storedButtonState){
    
    //if the button state has changed and it is on open the flow gate
    if (buttonState == 1){
      
      //calculate how long to turn the motor on via the potenitometer position, then multiply it by the maximum time to open flow gate
      turnTime = ((float)potValue/(float)1023) * (float)calTime;  

      moveMotor(turnTime, 0); //calls a subroutine to turn the motor, sending it the amount of time to turn and the direction to turn
      storedButtonState = buttonState;  //save the current button state as the stored button state
    
    }else{ //if the button is off (0) call the home subroutine to shut the flow gate, then save the current button state as the stored button state
      Home();
      storedButtonState = buttonState;
    }
  }

  //check to see if the potentiometer state has changed, the value has to change by more than 10 to trigger a move, this is to filter out vibrations caused by the lawn mower
  if (potValue < storedPotState - 10 || potValue > storedPotState + 10){
    if (potValue > storedPotState & buttonState == 1){  //if the pot value went up open the flow gate further by calling the moveMotor subroutine and commanding direction 0
      turnTime = (((float)potValue - storedPotState)/(float)1023) * (float)calTime;
      moveMotor(turnTime, 0);
      storedPotState = potValue;
    }else if (potValue < storedPotState & buttonState == 1){ //else if the pot value decreased close the flow gate by calling the moveMotor subroutine and commanding direction 1
      turnTime = ((storedPotState - (float)potValue)/(float)1023) * (float)calTime;
      moveMotor(turnTime, 1);
      storedPotState = potValue;
    }else{
      
    }
  }
  
}

void Home(){ //closes the flow gate until the proximity sensor input goes high
  switchState = digitalRead(swt);  //read the proximity sensor state

  digitalWrite(gled,HIGH);  //turn green led off
  digitalWrite(bled,LOW);  //turn blue led on to indicate that the flow gate is closing
  
  //Set direction to reverse
  digitalWrite(dir,HIGH);

  //Enable the Stepper Driver
  digitalWrite(EN,LOW);

  while (swtCount<5){  //the proximity sensor has to be high for 5 counts before the motor is turned off, this is for debounce

  analogWrite(stp, 250); //this turns the motor by setting the PWM output to 250 out of 255
  
  switchState = digitalRead(swt);  //reads state of proximity sensor

  //counts consecutive high states of the proximity sensor
  if (switchState==1){
    swtCount = swtCount + 1;
  }else{
    swtCount = 0;
  }
  }
  swtCount = 0; //reset proximity sensor count
  digitalWrite(EN,HIGH); //disable stepper motor
  analogWrite(stp, 0); //set stepper motor speed to 0
  
}

void CalCheck(){
  //Check to see if button is on during start-up
  buttonState = digitalRead(btn);
  potValue = analogRead(pot);

  // To enter calibration mode the button must be on at start-up and the pot needs to be turned to the max setting
  if (buttonState == 1 & potValue > 1000){
    Cal();  //Call the calibration routine if calibration criteria is met
  }else{
  }
}

//Subroutine to move the motor
void moveMotor(int tt, int d){ //tt is the time for the motor to run and d is direction
  if (d == 1){
    digitalWrite(dir,HIGH);
  }else{
    digitalWrite(dir,LOW);
  }

  //Enable the Stepper Driver
  digitalWrite(EN,LOW);

  //Set the motor speed
  analogWrite(stp, 250);

  //Leave the motor on for the specified time
  delay(tt);

  //Disable the Stepper Driver
  digitalWrite(EN,HIGH);

  //Set the motor speed to zero
  analogWrite(stp, 0);
  
}

//Calibration subroutine
void Cal(){

  digitalWrite(gled,HIGH);  //Turn green led off
  digitalWrite(bled,LOW);  //Turn blue led on to indicate calibration mode
  
  buttonState = digitalRead(btn);

  //Set direction to forward
  digitalWrite(dir,LOW);

  //Enable the Stepper Driver
  digitalWrite(EN,LOW);

  int secCount = 0;
  int milCount = 0;

  //While the button is on turn the motor
  while (buttonState==1){
  //Turn Motor
  analogWrite(stp, 250);

  buttonState = digitalRead(btn);
  delay(5);
  milCount = milCount + 1;  //Count the number of milliseconds that the motor runs
    if (milCount == 200){
      secCount = secCount + 1;
      milCount = 0;
    }
  }

  //Disable Stepper Driver
  digitalWrite(EN,HIGH);
  analogWrite(stp, 0);

  //Write the number of seconds and milliseconds to EEPROM
  EEPROM.write(addr, secCount);
  EEPROM.write(addr1, milCount);

  //Read in the values
  readEEPROM();

 //Close the flow gate using the home routine
  Home();

}

void readEEPROM(){
  //Read the calibration setting from eeprom
  maxSec = EEPROM.read(addr);
  maxMil = EEPROM.read(addr1);
  Serial.println(maxSec);
  Serial.println(maxMil);

  calTime = (maxSec * 1000) + (maxMil * 5);
}

