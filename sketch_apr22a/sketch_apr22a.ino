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
#define ALARM_SET_BUTTON 7
#define TEMP_DISPLAY_BUTTON 8
#define TIMER_UP_BUTTON 9
#define TIMER_DOWN_BUTTON 10

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'} // Change '*' to reset
};
byte rowPins[ROWS] = {12, 11, 10, 9}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, A3, A2}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Global variables
TM1637Display display1(CLK1, DIO1); // Display 1: Time and Date
TM1637Display display2(CLK2, DIO2); // Display 2: Temperature or Timer
RTC_DS3231 rtc;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DateTime now;
bool alarmActive = false;
DateTime alarmTime;
bool tempDisplay = false;
unsigned long timerStart = 0;
int timerCount = 0;
bool timerRunning = false;
bool countdownMode = false;
bool colonState = false;

long milliseconds;  

unsigned long time_m = 0; // the variable to store minute
unsigned long time_h = 0; // the variable to store hour
unsigned long time_s = 0; // the variable to store second
unsigned long time_month = 0;
unsigned long time_date = 0;

unsigned long last_m = 0; // the variable to store the last updated minute
unsigned long last_s = 0; // the variable to store the last updated second


void setup() {
    milliseconds = 0;
   display1.setBrightness(0x0f);
   display2.setBrightness(0x0f);
   rtc.begin();
   sensors.begin();
   rtc.adjust(DateTime(2024, 4, 22, 18, 10, 00));
   pinMode(BUZZER_PIN, OUTPUT);
   pinMode(ALARM_SET_BUTTON, INPUT_PULLUP);
   pinMode(TEMP_DISPLAY_BUTTON, INPUT_PULLUP);
   pinMode(TIMER_UP_BUTTON, INPUT_PULLUP);
   pinMode(TIMER_DOWN_BUTTON, INPUT_PULLUP);
   
   Serial.begin(9600);
   if (!rtc.begin()) {
      Serial.println("Couldn't find RTC");
      while (1);
   }

   digitalWrite(BUZZER_PIN, HIGH);

}

void loop() {    

   milliseconds = millis() % 1000;

   DateTime now = rtc.now();
   
   time_h = now.hour();
   time_m = now.minute();
   time_s = now.second();
   time_month = now.month();
   time_date = now.day();
   

    unsigned long time = time_h * 100 + time_m;
    const uint8_t colonMask = 0b01000000;

   if (time_s != last_s) { // only update if changed
      Serial.println(now.day());
      Serial.println(now.month());
      Serial.println(now.year());
      Serial.println("\n");
      last_s = time_s;
   }

    unsigned time_md = time_date * 100 + time_month;

    if (milliseconds <= 500) {     
      display1.showNumberDec(time, true, 4, 0); // no colon
      display2.showNumberDec(time_md, true, 4, 0);
    } else {
      display1.showNumberDecEx(time, colonMask, true, 4, 0); // no colon
      display2.showNumberDecEx(time_md, colonMask, true, 4, 0); // no colon
    }

    char key = keypad.getKey();
    
    if (key != NO_KEY) {
      if (key == '*') { // Check if asterisk (*) key is pressed
        // Reset the display
        display2.showNumberDecEx(0, 0b01110000); // Display NULL on all four digits
      } else {
        // Convert char to int
        int value = key - '0';
        display2.showNumberDec(value);
      }
    }

}

void handleButtons() {
   static uint32_t lastDebounceTime = 0;
   uint32_t debounceDelay = 50;

   // Alarm Set/Cancel
   if (digitalRead(ALARM_SET_BUTTON) == LOW && millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      setAlarm();
   }

   // Temperature Display Toggle
   if (digitalRead(TEMP_DISPLAY_BUTTON) == LOW && millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      toggleTemperatureDisplay();
   }

   // Timer Count-up
   if (digitalRead(TIMER_UP_BUTTON) == LOW && millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      toggleTimer(true);
   }

   // Timer Count-down
   if (digitalRead(TIMER_DOWN_BUTTON) == LOW && millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      toggleTimer(false);
   }
}

void displayTimeDate() {
   if (!tempDisplay) {
      int displayValue = (now.hour() * 100) + now.minute();
      display1.showNumberDecEx(displayValue, 0b01000000, true); // Display time with colon
   }
   else {
      displayTemperature();
   }
}

void setAlarm() {
   // Toggle alarm setting mode; for simplicity assume static alarm time
   if (!alarmActive) {
      alarmTime = DateTime(now.year(), now.month(), now.day(), now.hour(), (now.minute() + 1) % 60);
      alarmActive = true;
      Serial.println("Alarm set for one minute from now.");
   }
   else {
      alarmActive = false;
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Alarm canceled.");
   }
}

void triggerAlarm() {
   digitalWrite(BUZZER_PIN, HIGH);
   delay(1000);  // Buzzer on for 1 second
   digitalWrite(BUZZER_PIN, LOW);
   alarmActive = false; // Disable alarm after triggering
}

void toggleTemperatureDisplay() {
   tempDisplay = !tempDisplay;
}

void displayTemperature() {
   sensors.requestTemperatures();
   float tempC = sensors.getTempCByIndex(0);
   int tempToShow = static_cast<int>(tempC * 10); // Show one decimal place
   display2.showNumberDecEx(tempToShow, 0x40, true); // Display temperature with one decimal
}

void toggleTimer(bool countUp) {
   countdownMode = !countUp;
   if (!timerRunning) {
      timerStart = millis();
      timerRunning = true;
   }
   else {
      timerRunning = false;
      displayTimer();
   }
}

void displayTimer() {
   int elapsedSeconds = (millis() - timerStart) / 1000;
   if (countdownMode) {
      timerCount = max(0, timerCount - elapsedSeconds);
   }
   else {
      timerCount += elapsedSeconds;
   }
   display2.showNumberDec(timerCount, false); // Display timer count
   timerStart = millis(); // Reset timer start
}
