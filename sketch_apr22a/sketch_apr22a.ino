#include <TM1637Display.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Keypad.h>

// Display pins
#define CLK1 2
#define DIO1 3
#define CLK2 4
#define DIO2 5

// RTC and Temperature Sensor
#define ONE_WIRE_BUS 11

// Buzzer and Buttons
#define BUZZER_PIN 6

// Define the frequencies for the notes
#define B3  246.94
#define C4  261.63
#define Csh4  277.18
#define D4  293.66
#define Dsh4  311.13
#define E4  329.63
#define F4  349.23
#define Fsh4  369.99
#define G4  392.00
#define Gsh4  415.30
#define A4  440.00
#define Ash4  466.16
#define B4  493.88
#define C5  523.25
#define Csh5  554.37
#define D5  587.33
#define Dsh5  622.25
#define E5  659.25
#define F5  698.46
#define Fsh5  739.99
#define G5  783.99
#define Gsh5  830.61
#define A5  880.00
#define Ash5  932.33
#define B5  987.77
#define C6  1046.50

// Define the note durations (in milliseconds)
#define Q 500
#define H (Q * 2)
#define W (Q * 4)

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

   if ((time_h == alarm_h) && (time_m == alarm_m) ){
      triggerAlarm();
   }

   if (time_s != last_s) { // only update if changed
      Serial.print(now.hour()); Serial.print(':'); Serial.print(now.minute()); Serial.print(':'); Serial.println(now.second());
      Serial.print(now.day()); Serial.print('/'); Serial.print(now.month()); Serial.print('/'); Serial.println(now.year());
      if (alarmActive == true){
        Serial.print("Alarn Time: "); Serial.print(alarm_h); Serial.print(':'); Serial.println(alarm_m);
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

   Serial.println(key);
  
   if (key != NO_KEY) {
      handleKeypadInput(key);
   }
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
         alarmActive = false;
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
          timeDisplay = true;
          tempDisplay = false;
          timerRunning = false;
          countdownMode = false;
          countupMode = false;
         break;
      case 'D':
//          timeDisplay = false;
//          timerRunning = false;
//          countdownMode = false;
//          countupMode = false;
//          readTimeFromKeypad();
         break;
      case '*':
         // Reset the display or any other desired functionality
         break;
      default:
         // Convert char to int and display it
//         int value = key - '0';
//         display2.showNumberDec(value);
         break;
   }
}

void setAlarm() {
  
  if(alarmActive == false){
  Serial.println("Alarm set for one minute from now.");
  int alarmTime = readTimeFromKeypad();
  alarm_m = alarmTime % 100;
  alarm_h = alarmTime / 100;

  

  alarmActive = true;
  }
  else {
    Serial.println("Alarm canceled!!!");
    alarmActive = false;
  }

  key = NO_KEY;
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

bool isValidTime(int hours, int minutes) {
  
  return (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59);
}

// Function to play a note
void playNote(float noteFrequency, int noteDuration) {
  if (noteFrequency == 0) {
    delay(noteDuration);
    return;
  }
  int period = 1000000 / noteFrequency;
  int duration = period * 0.9;
  for (long i = 0; i < noteDuration * 1000L; i += period * 2) {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(duration);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(duration);
  }
  delay(20);
}

void triggerAlarm() {
   // Define the notes of the melody (in Hz)
  float melodyNotes[] = {B3, C4, Csh4, D4, Dsh4, E4, F4, Fsh4, G4, Gsh4, A4, Ash4, B4, C5, Csh5, D5, Dsh5, E5, F5, Fsh5, G5, Gsh5, A5, Ash5, B5, C6, B5, Ash5, A5, Gsh5, G5, Fsh5, F5, E5, Dsh5, D5, Csh5, C5, B4};
  int noteDurations[] = {W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, H, H, H, H, H, H, H, H, H, H, H, H, H, H, H, H, H};
  
  // Play each note of the melody
  for (int i = 0; i < sizeof(melodyNotes) / sizeof(melodyNotes[0]); i++) {
    playNote(melodyNotes[i], noteDurations[i]);
  }
   
   // Disable alarm after triggering
   alarmActive = false;
}


void displayTemperature() {

   sensors.requestTemperatures();
   float tempC = sensors.getTempCByIndex(0);
   int tempToShow = static_cast<int>(tempC * 10); // Show one decimal place
   display2.showNumberDecEx(tempToShow, 0x40, true); // Display temperature with one decimal
   display1.clear();

}

void toggleTimer(bool countUp) {
   countdownMode = !countUp;
   if (!timerRunning) {
      timerStart = millis();
      timerRunning = true;
   } else {
      timerRunning = false;
      displayTimer();
   }
}

void displayTimer() {
   int elapsedSeconds = (millis() - timerStart) / 1000;
   if (countdownMode) {
      timerCount = max(0, timerCount - elapsedSeconds);
   } else {
      timerCount += elapsedSeconds;
   }
   display2.showNumberDec(timerCount, false); // Display timer count
   timerStart = millis(); // Reset timer start
}


void countUp() {
   int elapsedSeconds = (millis() - timerStart) / 1000;
      timerCount += elapsedSeconds;
   display2.showNumberDec(timerCount, false); // Display timer count
   timerStart = millis(); // Reset timer start
}
