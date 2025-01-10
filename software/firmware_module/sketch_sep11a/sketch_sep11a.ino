#include <Stepper.h>


#define BAUDRATE 115200

#define STEPPERPIN1 11
#define STEPPERPIN2 10
#define STEPPERPIN3 9
#define STEPPERPIN4 8
#define STEPS 2038 //28BYJ-48 stepper, number of steps
#define HALLPIN 7 //Pin of hall sensor
#define AMOUNTFLAPS 45

#define ROTATIONDIRECTION 1 //-1 for reverse direction
#define OVERHEATINGTIMEOUT 2 //timeout in seconds to avoid overheating of stepper. After starting rotation, the counter will start. Stepper won't move again until timeout is passed
unsigned long lastRotation = 0;

//globals
int displayedLetter = 0; //currently shown letter
int desiredLetter = 0; //letter to be shown
const String letters[] = {" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Ä", "Ö", "Ü", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ".", "-", "?", "!"};
Stepper stepper(STEPS, STEPPERPIN1, STEPPERPIN3, STEPPERPIN2, STEPPERPIN4); //stepper setup
bool lastInd1 = false; //store last status of phase
bool lastInd2 = false; //store last status of phase
bool lastInd3 = false; //store last status of phase
bool lastInd4 = false; //store last status of phase
float missedSteps = 0; //cummulate steps <1, to compensate via additional step when reaching >1
int currentlyrotating = 0; // 1 = drum is currently rotating, 0 = drum is standing still
int stepperSpeed = 10; //current speed of stepper, value only for first homing

int eeAddress = 0;   //EEPROM address for calibration offset
int calOffset;       //Offset for calibration in steps, stored in EEPROM, gets read in setup
int receivedNumber = 0;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUDRATE);
  Serial.println("starting unit");
  stepperSpeed = 17; //until homing is implemented
}

void loop() {
  // put your main code here, to run repeatedly:
  int calLetters[10] = {0, 26, 1, 21, 14, 43, 30, 31, 32, 39};
  for (int i = 0; i < 10; i++) {
    int currentCalLetter = calLetters[i];
    rotateToLetter(currentCalLetter);
    delay(5000);
  }
}

//rotate to letter
void rotateToLetter(int toLetter) {
  if (lastRotation == 0 || (millis() - lastRotation > OVERHEATINGTIMEOUT * 1000)) {
    lastRotation = millis();
    //get letter position
    int posLetter = -1;
    posLetter = toLetter;
    int posCurrentLetter = -1;
    posCurrentLetter = displayedLetter;
    //int amountLetters = sizeof(letters) / sizeof(String);
#ifdef serial
    Serial.print("go to letter: ");
    Serial.println(letters[toLetter]);
#endif
    //go to letter, but only if available (>-1)
    if (posLetter > -1) { //check if letter exists
      //check if letter is on higher index, then no full rotaion is needed
      if (posLetter >= posCurrentLetter) {
#ifdef serial
        Serial.println("direct");
#endif
        //go directly to next letter, get steps from current letter to target letter
        int diffPosition = posLetter - posCurrentLetter;
        startMotor();
        stepper.setSpeed(stepperSpeed);
        //doing the rotation letterwise
        for (int i = 0; i < diffPosition; i++) {
          float preciseStep = (float)STEPS / (float)AMOUNTFLAPS;
          int roundedStep = (int)preciseStep;
          missedSteps = missedSteps + ((float)preciseStep - (float)roundedStep);
          if (missedSteps > 1) {
            roundedStep = roundedStep + 1;
            missedSteps--;
          }
          stepper.step(ROTATIONDIRECTION * roundedStep);
        }
      }
      else {
        //full rotation is needed, good time for a calibration
#ifdef serial
        Serial.println("full rotation incl. calibration");
#endif
        //calibrate(false); //calibrate revolver and do not stop motor
        //startMotor();
        stepper.setSpeed(stepperSpeed);
        for (int i = 0; i < posLetter; i++) {
          float preciseStep = (float)STEPS / (float)AMOUNTFLAPS;
          int roundedStep = (int)preciseStep;
          missedSteps = missedSteps + (float)preciseStep - (float)roundedStep;
          if (missedSteps > 1) {
            roundedStep = roundedStep + 1;
            missedSteps--;
          }
          stepper.step(ROTATIONDIRECTION * roundedStep);
        }
      }
      //store new position
      displayedLetter = toLetter;
      //rotation is done, stop the motor
      delay(100); //important to stop rotation before shutting of the motor to avoid rotation after switching off current
      stopMotor();
    }
    else {
#ifdef serial
      Serial.println("letter unknown, go to space");
#endif
      desiredLetter = 0;
    }
  }
}

//switching off the motor driver
void stopMotor() {
  lastInd1 = digitalRead(STEPPERPIN1);
  lastInd2 = digitalRead(STEPPERPIN2);
  lastInd3 = digitalRead(STEPPERPIN3);
  lastInd4 = digitalRead(STEPPERPIN4);

  digitalWrite(STEPPERPIN1, LOW);
  digitalWrite(STEPPERPIN2, LOW);
  digitalWrite(STEPPERPIN3, LOW);
  digitalWrite(STEPPERPIN4, LOW);
#ifdef serial
  Serial.println("Motor Stop");
#endif
  currentlyrotating = 0; //set active state to not active
  delay(100);
}

void startMotor() {
#ifdef serial
  Serial.println("Motor Start");
#endif
  currentlyrotating = 1; //set active state to active
  digitalWrite(STEPPERPIN1, lastInd1);
  digitalWrite(STEPPERPIN2, lastInd2);
  digitalWrite(STEPPERPIN3, lastInd3);
  digitalWrite(STEPPERPIN4, lastInd4);
}


