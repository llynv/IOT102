#include <TM1637Display.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Keypad.h>
#include "pitches.h"


// Display pins
#define CLK1 2
#define DIO1 3
#define CLK2 4
#define DIO2 5

// RTC and Temperature Sensor
#define ONE_WIRE_BUS 11

// Buzzer and Buttons
#define BUZZER_PIN 6

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'} // Change '*' to reset
};
byte rowPins[ROWS] = {13, 12, 11, 10}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, A3, A2}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Global variables
TM1637Display display1(CLK1, DIO1); // Display 1: Time and Date
TM1637Display display2(CLK2, DIO2); // Display 2: Temperature or Timer
RTC_DS3231 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DateTime now;

unsigned long timerStart = 0;
int timerCount = 0;

bool timeDisplay = true;
bool tempDisplay = false;
bool alarmActive = false;
bool alarmTrigger = false;
bool timerRunning = false;
bool countdownMode = false;
bool countupMode = false;
bool colonState = false;

long milliseconds;

unsigned long time_m = 0; // the variable to store minute
unsigned long time_h = 0; // the variable to store hour
unsigned long time_s = 0; // the variable to store second
unsigned long time_month = 0;
unsigned long time_date = 0;

unsigned long alarm_m = 0; // the variable to store alarm minute
unsigned long alarm_h = 0; // the variable to store alarm hour

unsigned long count_m = 0; // the variable to store count minute
unsigned long count_s = 0; // the variable to store alarm second

unsigned long last_m = 0; // the variable to store the last updated minute
unsigned long last_s = 0; // the variable to store the last updated second

char key;

void setup() {
   milliseconds = 0;
   display1.setBrightness(0x0f);
   display2.setBrightness(0x0f);
   rtc.begin();
   sensors.begin();
   pinMode(BUZZER_PIN, OUTPUT);

   Serial.begin(9600);
   if (!rtc.begin()) {
      Serial.println("Couldn't find RTC");
      while (1);
   }

   digitalWrite(BUZZER_PIN, HIGH);
}

void loop(){ 
  
  
   milliseconds = millis() % 1000;

   DateTime now = rtc.now();

   time_h = now.hour();
   time_m = now.minute();
   time_s = now.second();
   time_month = now.month();
   time_date = now.day();

   unsigned long time = time_h * 100 + time_m;
   const uint8_t colonMask = 0b01000000;

   if (time_h == alarm_h && time_m == alarm_m && alarmTrigger) {
      triggerAlarm();
      alarmTrigger ^= 1;
    }

    if (countupMode == true){
      countUp();
    }

     if (countdownMode) {
      displayTimer();
  }

   if (time_s != last_s) { // only update if changed
      Serial.print(now.hour()); Serial.print(':'); Serial.print(now.minute()); Serial.print(':'); Serial.println(now.second());
      Serial.print(now.day()); Serial.print('/'); Serial.print(now.month()); Serial.print('/'); Serial.println(now.year());
      if (alarmActive == true){
        Serial.print("Alarm Time: "); Serial.print(alarm_h); Serial.print(':'); Serial.println(alarm_m);
      }
      Serial.println("\n");
      last_s = time_s;
   }


   if (timeDisplay == true && tempDisplay == false){
      display_Clock();
   }
   
   unsigned time_md = time_date * 100 + time_month;

   char tmp = keypad.getKey();
   key = (tmp == NO_KEY ? key : tmp);
  
   if (key != NO_KEY) {
      handleKeypadInput(key);
   }
}

void enterAlarmMode() {
  char key = keypad.getKey();
  
}

void display_Clock(){
  milliseconds = millis() % 1000;

   DateTime now = rtc.now();

   time_h = now.hour();
   time_m = now.minute();
   time_s = now.second();
   time_month = now.month();
   time_date = now.day();

   unsigned long time = time_h * 100 + time_m;
   const uint8_t colonMask = 0b01000000;

   unsigned time_md = time_date * 100 + time_month;

   if (milliseconds <= 500) {
      display1.showNumberDec(time, true, 4, 0); // no colon
      display2.showNumberDec(time_md, true, 4, 0);
   } else {
      display1.showNumberDecEx(time, colonMask, true, 4, 0); // colon
      display2.showNumberDecEx(time_md, colonMask, true, 4, 0); //
   }
}

void handleKeypadInput(char key) {
    switch (key) {
        case 'A':
            setAlarm();
            break;
        case 'B':
            timeDisplay = false;
            tempDisplay = true;
            timerRunning = false;
            countdownMode = false;
            countupMode = false;
            displayTemperature();
            break;
        case 'C':
            timeDisplay = false;
            tempDisplay = false;
            timerRunning = false;
            countdownMode = false;
            countupMode = true;
            timerStart = millis();  // Reset and start the timer
            timerCount = 0;         // Reset timer count
            break;
        case 'D':
            timeDisplay = false;
            tempDisplay = false;
            timerRunning = false;
            countdownMode = true;
            countupMode = false;
            break;
        case '*':
            // Reset functionality
            timeDisplay = true;
            tempDisplay = false;
            timerRunning = false;
            countdownMode = false;
            countupMode = false;
            break;
      default:
         // Convert char to int and display it
//         int value = key - '0';
//         display2.showNumberDec(value);
         break;
   }
}

int readTimeFromKeypad() {
  Serial.println("Enter read from keypad");
  display2.showNumberDec(0, false);
  char number[5]; // Array to store the four digits and null terminator
  int digitPos = 0; // Counter for the number of digits entered
  char readKey = keypad.getKey();
  unsigned long alarmHour = 0, alarmMinute = 0, alarmTime = 0;
  while (true) {
    if (digitPos > 3) break;
    readKey = keypad.getKey();
    if (readKey != NO_KEY) {
      if (digitPos < 2) {
        alarmHour = alarmHour * 10 + (readKey - '0');
      } else {
        alarmMinute = alarmMinute * 10 + (readKey - '0');
      }
      alarmTime = alarmHour * 100 + alarmMinute;
      digitPos++;
      display2.showNumberDec(alarmTime, false, 4, 0); // Display the number with leading zeros suppressed
    }
  } return alarmTime;
  key = NO_KEY;
}

void setAlarm() {
  Serial.print("alarm : ");
  Serial.println(alarmActive);
  int alarmTime = 0;
  key = NO_KEY;
  
  if (alarmActive) {
    alarmActive = false;
    alarmTrigger = false;
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }
 
  DateTime now = rtc.now();
  Serial.println("Alarm set for one minute from now.");

  do{
  alarmTime = readTimeFromKeypad();
  alarm_m = alarmTime % 100;
  alarm_h = alarmTime / 100;
  alarmActive = true;
  alarmTrigger = true;
  }while(!isValidTime(alarm_h, alarm_m));

}

bool isValidTime(int hours, int minutes) {
  
  return (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59);
}

bool isValidTimer(int minutes, int second) {
  
  return (minutes >= 0 && minutes <= 59 && second >= 0 && second <= 59);
}

void triggerAlarm() {
    // alarm melody define
  int melody[] = {
  NOTE_E5, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_B4,
  NOTE_A4, NOTE_A4, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_C5, NOTE_A4, NOTE_A4, NOTE_A4, NOTE_B4, NOTE_C5,
  
  NOTE_D5, NOTE_F5, NOTE_A5, NOTE_G5, NOTE_F5,
  NOTE_E5, NOTE_C5, NOTE_E5, NOTE_D5, NOTE_C5,
  NOTE_B4, NOTE_B4, NOTE_C5, NOTE_D5, NOTE_E5,
  NOTE_C5, NOTE_A4, NOTE_A4, REST
  };

  int durations[] = {
    4, 8, 8, 4, 8, 8,
    4, 8, 8, 4, 8, 8,
    4, 8, 4, 4,
    4, 4, 8, 4, 8, 8,
    
    4, 8, 4, 8, 8,
    4, 8, 4, 8, 8,
    4, 8, 8, 4, 4,
    4, 4, 4, 4
  };
  
    int size = sizeof(durations) / sizeof(int);

    for (int note = 0; note < size; note++) {
      //to calculate the note duration, take one second divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int duration = 1000 / durations[note];
      tone(BUZZER_PIN, melody[note], duration);

    //to distinguish the notes, set a minimum time between them.
    //the note's duration + 30% seems to work well:
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    
    //stop the tone playing:
    noTone(BUZZER_PIN);
  }
}

void displayTemperature() {

   const uint8_t SEG_DEGREE = 0x63;  // You might need to adjust this based on your display

   // Segment bits for 'C': (A, D, E, F)
   const uint8_t SEG_Ce = 0x39;

   // Prepare the data array to be sent to the display
   uint8_t displayData[4] = {0, 0, 0, 0};  // Clear all digits first
  
   // Place '°' and 'C' in the last two digits, respectively
   displayData[2] = SEG_DEGREE; // Set '°' in the third position
   displayData[3] = SEG_Ce;      // Set 'C' in the fourth position
   
   int temperature = rtc.getTemperature();
   display2.showNumberDec(temperature, false);
   display1.setSegments(displayData);

}

void countUp() {
    key = NO_KEY;
    if (countupMode) {
        unsigned long currentMillis = millis();
        int elapsedSeconds = (currentMillis - timerStart) / 1000;
        timerCount = elapsedSeconds;  // Update timer count based on elapsed time

        display1.showNumberDec(timerCount, false); // Display timer count with leading zeros

        // Display "UP" or similar indicator to show count-up mode is active
        uint8_t data[] = { 0x3E, 0x73, 0x00, 0x00 }; // Segments to display "UP  "
        display2.setSegments(data);

       if(timerCount % 60 == 0 && timerCount != 0){
          digitalWrite(BUZZER_PIN, LOW);
          delay(250);
          digitalWrite(BUZZER_PIN, HIGH);
          delay(250);
          digitalWrite(BUZZER_PIN, LOW);
          delay(250);
          digitalWrite(BUZZER_PIN, HIGH);
       }
    }
}

int readTimerFromKeypad() {
  Serial.println("Enter read from keypad");
  display2.showNumberDec(0, false);
  char number[5]; // Array to store the four digits and null terminator
  int digitPos = 0; // Counter for the number of digits entered
  char readKey = keypad.getKey();
  unsigned long countSecond = 0, countMinute = 0, countTimer = 0;
  while (true) {
    if (digitPos > 3) break;
    readKey = keypad.getKey();
    if (readKey != NO_KEY) {
      if (digitPos < 2) {
        countMinute = countMinute * 10 + (readKey - '0');
      } else {
        countSecond = countSecond * 10 + (readKey - '0');
      }
      countTimer = countMinute * 100 + countSecond;
      digitPos++;
      display2.showNumberDec(countTimer, false, 4, 0); // Display the number with leading zeros suppressed
    }
  } return countTimer;
  key = NO_KEY;
}

void setTimer() {
  Serial.print("Timer : ");
  Serial.println(alarmActive);
  int countTimer = 0;
  key = NO_KEY;
  
  if (countdownMode) {
    countdownMode = false;
    alarmTrigger = false;
    digitalWrite(BUZZER_PIN, HIGH);
    return;
  }
 
  DateTime now = rtc.now();
  Serial.println("Timer set up complete!.");

  do{
  countTimer = readTimerFromKeypad();
  count_m = countTimer % 100;
  count_s = countTimer / 100;
  countdownMode = true;
  alarmTrigger = true;
  }while(!isValidTimer(count_m, count_s));

}

void displayTimer() {
  key = NO_KEY;
  if (countdownMode) {
    int elapsedSeconds = (millis() - timerStart) / 1000;
    int remainingTime = max(0, timerCount - elapsedSeconds);
    uint8_t seg_Dd = SEG_B | SEG_C | SEG_D | SEG_E | SEG_G;
    uint8_t seg_Oo = SEG_C | SEG_D | SEG_E | SEG_G;
    uint8_t seg_Ww = SEG_D | SEG_E | SEG_C ;
    uint8_t seg_Nn = SEG_C | SEG_E | SEG_G;
    uint8_t data[] = { seg_Dd, seg_Oo, seg_Ww, seg_Nn };
    
    if (remainingTime == 0) {
      countdownMode = false; // Stop countdown when it reaches 0
      digitalWrite(BUZZER_PIN, LOW);
      delay(250);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(250);
      digitalWrite(BUZZER_PIN, LOW);
      delay(250);
      digitalWrite(BUZZER_PIN, HIGH);
    }
    display1.showNumberDec(remainingTime, true); // Display remaining time
    display2.setSegments(data);
  }
}
