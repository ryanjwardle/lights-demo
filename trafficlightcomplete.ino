//4 way intersection (North/South street, East/West street), with 4 sets of traffic signals (colors R, Y, G
const int NSLights[] = { 5, 6, 7 }; // North/South red, yellow, green LEDs at pins 5, 6, 7
const int WLights[] = { 8, 9, 10 }; // West red, yellow, green LEDs at pins 8, 9, 10
const int ELights[] = { 11, 12, 13 };// East red, yellow, green LEDs at pins 11, 12, 13
const int crosswalkButtons = 0; // crosswalk buttons at pin 1
const int stopLight = 3; // do not cross LED at pin 3
const int walkLight = 4; // walk LED at pin 4
const int emergency = 2; // emergency override at pin 2
bool latch1 = false; // crosswalk latch to lock walk LED in place
bool latch2 = false; //crosswalk latch to lock walk LED in place
bool interruptFlag = false; // interrupt flag to trigger emergency override function

struct buttonLog {  //Create a structure buttonLog that will store  variables
  int readingTime;
} crosswalkLog[10], emergencyLog[10]; // readings for daily crosswalk and emergency button usage

/*************************************************************************
  actual interval lengths over 24 hours
  const int midnightToMornCycleLength = (7*60*1000);  // midnight-7am
  const int mornCycleLength = (2*60*1000);  // 7-9 am
  const int middayCycleLength = (7*60*1000);  // 9am-4pm
  const int eveningCycleLength = (2*60*1000);  // 4-6pm
  const int eveningToOvernightCycleLength = (6*60*1000);  // 6pm-midnight
  const int dailyInterval = 24*60*1000;
**************************************************************************/

// interval times for various cycles of traffic operation
// scaled for simulation where 1 hr = 1000ms
const int midnightToMornCycleLength = 7000;
const int mornCycleLength = 2000;
const int middayCycleLength = 7000;
const int eveningCycleLength = 2000;
const int eveningToOvernightCycleLength = 6000;
const int dailyInterval = 3000;

unsigned long previousTime = 0;
unsigned long prevFuncTime = 0;
unsigned long prevMiddayTime = 0;

unsigned long crosswalkCount = 0; // number of daily readings for crosswalk button pushes
unsigned long emergencyCount = 0; // numner of daily readings for emergency count pushes

void setup() {
  Serial.begin(9600); // initializing serial monitor
  pinMode(emergency, INPUT); // emergency override button
  pinMode(crosswalkButtons, INPUT); // crosswalk pushbuttons
  pinMode(stopLight, OUTPUT); // crosswalk stop led
  pinMode(walkLight, OUTPUT); // crosswalk walk led
  attachInterrupt(0, emergencyOverride, RISING); // attach emergency override interrupt
  // set LED array pins as outputs
  for (int i = 0; i < 3; i++) {
    pinMode(NSLights[i], OUTPUT);
    pinMode(ELights[i], OUTPUT);
    pinMode(WLights[i], OUTPUT);
  }
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - previousTime;

  /****************************************************************
    if full 24 hour interval has passed, reset daily elapsed time
  *****************************************************************/
  if (elapsedTime >= dailyInterval) {
    previousTime = currentTime;
    exportData(); // Export data to serial monitor
    crosswalkCount = 0; //reset crosswalk counter
    emergencyCount = 0; //reset emergency counter
    for (int i = 0; i < 10; i++) {
      emergencyLog[i].readingTime = 0;
	  crosswalkLog[i].readingTime = 0;
  	}
  }
  /****************************************************************/

  // Loop through each light sequence based on time of day
  if (elapsedTime <= midnightToMornCycleLength) { // equal to 7 hrs midnight to 7am
    middayOvernightCycle();// if midnight to 7 am, call middayOvernightCycle
  } else if ((elapsedTime > midnightToMornCycleLength) && (elapsedTime <= midnightToMornCycleLength + mornCycleLength)) { // equal to 7 hrs midnight to 7am plus 2hrs 7-9am
    mornEveCycle();  // if 7am-9am, call morning/evening cycle
  } else if ((elapsedTime > midnightToMornCycleLength + mornCycleLength) && (elapsedTime <= midnightToMornCycleLength + mornCycleLength + middayCycleLength)) { // equal to 7hrs midnight to 7 plus 2hrs 7-9 plus 7hrs 9am-4pm
    middayOvernightCycle();  // if 9am-4pm, call midday/overnight cycle
  } else if ((elapsedTime > midnightToMornCycleLength + mornCycleLength + middayCycleLength) && (elapsedTime <= midnightToMornCycleLength + mornCycleLength + middayCycleLength + eveningCycleLength)) { // equal to 7hrs midnight to 7 plus 2hrs 7-9 plus 7hrs 9am-4pm plus 2hrs 4-6pm
    mornEveCycle();  // if 4pm-6pm, call morning/evening cycle
  } else if ((elapsedTime > midnightToMornCycleLength + mornCycleLength + middayCycleLength + eveningCycleLength) && elapsedTime < dailyInterval) { // where dailyInterval is = to the full 24hr period
    middayOvernightCycle();  // if 6pm-7am, call midday/overnight cycle
  }

  // if crosswalk button pressed
  if (digitalRead(crosswalkButtons) == HIGH ) {
    delay(200);
    latch1 = true;
    latch2 = true;
    crosswalkLog[crosswalkCount].readingTime = elapsedTime;
    crosswalkCount++;
  }

  // if emergency override triggered
  if (interruptFlag == HIGH) {
    emergencyLog[emergencyCount].readingTime = elapsedTime;
    emergencyCount++;
    emergencyLightSequence();
    interruptFlag = LOW;
  }
}
     
/***************************************************
  Morning/Evening Light Cycle (7am-9am & 4pm-6pm):
****************************************************/
void mornEveCycle() {
  unsigned long currFuncTime = millis();
  unsigned long elapsedFunc = currFuncTime - prevFuncTime;

  // reset elapsed function time after each run through cycle
  if (elapsedFunc > 4000) { // the length of time it takes to run through ONE iteration of this cycle
    prevFuncTime = currFuncTime;
  }

  // EW Green/NS Red (60s) (CROSSWALK BUTTON PRESSED)
  if (((elapsedFunc <= (1000)) && ((latch1 == true) || (elapsedFunc <= (1000)) && (latch2 == true)))) {
    digitalWrite(NSLights[0], HIGH); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], LOW); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], HIGH); // green
    digitalWrite(WLights[0], LOW); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], HIGH); // green

    digitalWrite(walkLight, HIGH);// walk
    digitalWrite(stopLight, LOW);// do not cross
    latch1 = true;
    latch2 = false;
  }

  // EW Green/NS Red (60s) (CROSSWALK BUTTON NOT PRESSED)
  if ((elapsedFunc <= (1000)) && (latch1 == false)) {
    digitalWrite(NSLights[0], HIGH); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], LOW); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], HIGH); // green
    digitalWrite(WLights[0], LOW); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], HIGH); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); //do not cross
  }

  // EW Yellow/NS Red (3s)
  if ((elapsedFunc > (1000)) && (elapsedFunc <= (2000))) {
    digitalWrite(NSLights[0], HIGH); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], LOW); // red
    digitalWrite(ELights[1], HIGH); // yellow
    digitalWrite(ELights[2], LOW); // green
    digitalWrite(WLights[0], LOW); // red
    digitalWrite(WLights[1], HIGH); // yellow
    digitalWrite(WLights[2], LOW); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); //do not cross
    latch1 = false;
  }

  // NS Green/EW Red (20s)
  if ((elapsedFunc > (2000)) && (elapsedFunc <= (3000))) {
    digitalWrite(NSLights[0], LOW); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], HIGH); // green
    digitalWrite(ELights[0], HIGH); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], LOW); // green
    digitalWrite(WLights[0], HIGH); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], LOW); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); //do not cross
  }
  // NS Yellow/EW Red (3s)
  if ((elapsedFunc > (3000)) && elapsedFunc <= (4000)) {
    digitalWrite(NSLights[0], LOW); // red
    digitalWrite(NSLights[1], HIGH); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], HIGH); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], LOW); // green
    digitalWrite(WLights[0], HIGH); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], LOW); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); // do not cross
  }
}
/****************************************************
  Midday/Overnight Light Cycle (9am-4pm & 6pm-7am):
*****************************************************/

void middayOvernightCycle() {
  unsigned long currMiddayTime = millis();
  unsigned long elapsedMidday = currMiddayTime - prevMiddayTime;

  // reset elapsed function time after each run through cycle
  if (elapsedMidday > 4000) { // equal to the length of time it takes to run through ONE iteration of this cycle
    prevMiddayTime = currMiddayTime;
  }

  // EW Green/NS Red (30s) (CROSSWALK BUTTON PRESSED)
  if (((elapsedMidday <= (1000)) && ((latch1 == true) || (elapsedMidday <= (1000)) & (latch2 == true)))) {
    digitalWrite(NSLights[0], HIGH); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], LOW); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], HIGH); // green
    digitalWrite(WLights[0], LOW); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], HIGH); // green

    digitalWrite(walkLight, HIGH); // walk
    digitalWrite(stopLight, LOW); // do not cross
    latch1 = true;
    latch2 = false;
  }

  // EW Green/NS Red (30s) (CROSSWALK NOT PRESSED)
  if ((elapsedMidday <= (1000)) && (latch1 == false)) {
    digitalWrite(NSLights[0], HIGH); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], LOW); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], HIGH); // green
    digitalWrite(WLights[0], LOW); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], HIGH); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); // do not cross
  }

  // EW Yellow/NS Red (3s)
  if ((elapsedMidday > (1000)) && (elapsedMidday <= (2000))) {
    digitalWrite(NSLights[0], HIGH); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], LOW); // red
    digitalWrite(ELights[1], HIGH); // yellow
    digitalWrite(ELights[2], LOW); // green
    digitalWrite(WLights[0], LOW); // red
    digitalWrite(WLights[1], HIGH); // yellow
    digitalWrite(WLights[2], LOW); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); // do not cross
    latch1 = false;
  }

  // NS Green/EW Red (30s)
  if ((elapsedMidday > (2000)) && (elapsedMidday <= (3000))) {
    digitalWrite(NSLights[0], LOW); // red
    digitalWrite(NSLights[1], LOW); // yellow
    digitalWrite(NSLights[2], HIGH); // green
    digitalWrite(ELights[0], HIGH); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], LOW); // green
    digitalWrite(WLights[0], HIGH); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], LOW); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); // do not cross
  }

  // NS Yellow/EW Red (3s)
  if ((elapsedMidday > (3000)) && (elapsedMidday <= (4000))) {
    digitalWrite(NSLights[0], LOW); // red
    digitalWrite(NSLights[1], HIGH); // yellow
    digitalWrite(NSLights[2], LOW); // green
    digitalWrite(ELights[0], HIGH); // red
    digitalWrite(ELights[1], LOW); // yellow
    digitalWrite(ELights[2], LOW); // green
    digitalWrite(WLights[0], HIGH); // red
    digitalWrite(WLights[1], LOW); // yellow
    digitalWrite(WLights[2], LOW); // green

    digitalWrite(walkLight, LOW); // walk
    digitalWrite(stopLight, HIGH); // do not cross
  }
}

/*********************************************************************
  Emergency light sequence to run when emerg override button pressed
**********************************************************************/
void emergencyLightSequence() {
    if (digitalRead(NSLights[0]) == LOW) {
      digitalWrite(NSLights[0], LOW); // red
      digitalWrite(NSLights[1], HIGH); // yellow
      digitalWrite(NSLights[2], LOW); // green
      digitalWrite(ELights[0], HIGH); // red
      digitalWrite(ELights[1], LOW); // yellow
      digitalWrite(ELights[2], LOW); // green
      digitalWrite(WLights[0], HIGH); // red
      digitalWrite(WLights[1], LOW); // yellow
      digitalWrite(WLights[2], LOW); // green

      digitalWrite(walkLight, LOW); // walk
      digitalWrite(stopLight, HIGH); // do not cross
      delay(1000);
      digitalWrite(NSLights[0], HIGH); // red
      digitalWrite(NSLights[1], LOW); // yellow
      digitalWrite(ELights[2], HIGH); // green
    }
    if  (digitalRead(NSLights[0]) == HIGH) {
      digitalWrite(NSLights[0], HIGH); // red
      digitalWrite(NSLights[1], LOW); // yellow
      digitalWrite(NSLights[2], LOW); // green
      digitalWrite(ELights[0], LOW); // red
      digitalWrite(ELights[1], LOW); // yellow
      digitalWrite(ELights[2], HIGH); // green
      digitalWrite(WLights[0], LOW); // red
      digitalWrite(WLights[1], HIGH); // yellow
      digitalWrite(WLights[2], LOW); // green

      digitalWrite(walkLight, LOW); //walk
      digitalWrite(stopLight, HIGH); //do not cross
      delay(1000);
      digitalWrite(WLights[0], HIGH); // red
      digitalWrite(WLights[1], LOW); // yellow
      delay(3000);
      
    }
}

/****************************************************************************
  Emergency Override: On button press emergency vehicle activated override.
  Forces to G in E direction without skipping Y in N/S direction.
  Fire station located to west, override for vehicles heading east
  
  Triggers flag which executes emergency code
*****************************************************************************/
void emergencyOverride() {
  interruptFlag = HIGH;
}

/****************************************************
  Export crosswalk and emergency button occurrences
****************************************************/
void exportData() {
  Serial.println("Crosswalk Button Times Pressed: ");
  for (int i = 0; i < 10; i++) {
    if ((crosswalkLog[i].readingTime) > 0) {
      Serial.println(crosswalkLog[i].readingTime);
    }
  }

  Serial.println("Emergency Button Times Pressed: ");
  for (int i = 0; i < 10; i++) {
    if ((emergencyLog[i].readingTime) > 0) {
      Serial.println(emergencyLog[i].readingTime);
    }
  }
  Serial.println(" ");
  Serial.println("Total Number of CrossWalk Buttons Pressed: ");
  Serial.print(crosswalkCount);
  Serial.println(" ");
  Serial.println("Total Number of Emergency Buttons Pressed: ");
  Serial.print(emergencyCount);
  Serial.println(" ");
}