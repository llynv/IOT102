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
DateTime alarmTime;

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

   if ((time_h == alarmTime.hour()) && (time_m == alarmTime.minute()) ){
      triggerAlarm();
   }

   if (time_s != last_s) { // only update if changed
      Serial.print(now.hour()); Serial.print(':'); Serial.print(now.minute()); Serial.print(':'); Serial.println(now.second());
      Serial.print(now.day()); Serial.print('/'); Serial.print(now.month()); Serial.print('/'); Serial.println(now.year());
      if (alarmActive == true){
        Serial.print("Alarn Time: "); Serial.print(alarmTime.hour()); Serial.print(':'); Serial.println(alarmTime.minute());
      }
      Serial.println("\n");
      last_s = time_s;
   }



   if (timeDisplay == true && tempDisplay == false){
      display_Clock();
   }
   
   unsigned time_md = time_date * 100 + time_month;
   char key = keypad.getKey();

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
          timeDisplay = false;
          tempDisplay = false;
          timerRunning = false;
          countdownMode = false;
          countupMode = false;
          countUp();
         break;
      case '*':
         // Reset the display or any other desired functionality
         break;
      default:
         // Convert char to int and display it
         int value = key - '0';
         display2.showNumberDec(value);
         break;
   }
}

void setAlarm() {
  
  if(alarmActive == false){
  Serial.println("Alarm set for one minute from now.");
  alarmTime = DateTime(now.year(), now.month(), now.day(), now.hour(), (now.minute() + 1) % 60);
  alarm_m = alarmTime.minute();
  alarm_h = alarmTime.hour();
  alarmActive = true;
  }
  else {
    Serial.println("Alarm canceled!!!");
    alarmActive = false;
  }
}

void triggerAlarm() {
   // Define the notes of the melody (in Hz)
   int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
   
   // Define the duration of each note (in milliseconds)
   int noteDurations[] = {200, 200, 200, 200, 200, 200, 200, 200};
   
   // Play the melody
   for (int i = 0; i < 8; i++) {
      tone(BUZZER_PIN, melody[i], noteDurations[i]);
      delay(noteDurations[i] * 1.3); // Add a small delay between notes for better separation
   }
   
   // Turn off the buzzer
   noTone(BUZZER_PIN);
   
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
