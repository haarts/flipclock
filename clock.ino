#include <Wire.h>
#include <avr/sleep.h>
#include "RTClib.h"
#include "Ports.h"

RTC_DS1307 RTC;

/*EMPTY_INTERRUPT(WDT_vect);*/
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

int lastSeenMinute;
int lastSeenSecond;

volatile bool interrupted = false;
DateTime interruptedAt;
bool interruptedAtSet = false;

#define skipOneHourPin 3
#define waitOneHourPin 4
#define skipOneMinutePin 5
#define waitOneMinutePin 6

struct {
  int minutesToWait;
  int minutesToSkip;
} buttonPresses;

// control pin 1 of the SN754410NE IC
#define hBridgeControlPin1 8
// control pin 2 of the SN754410NE IC
#define hBridgeControlPin2 9
// off/on pin of the SN754410NE IC
#define hBridgeEnablePin 10
// listening pin to go in to clock programming mode
#define interruptPin 2

void setup() {
  Serial.begin(9600);

  //Set up clock
  Wire.begin();
  RTC.begin();

  if( !RTC.isrunning()){
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  lastSeenMinute = getCurrentMinute();
  lastSeenSecond = getCurrentSecond();

  //set up interrupt
  attachInterrupt(0, registerInterrupt, FALLING);
  pinMode(interruptPin       , INPUT);

  //set up hBridge
  pinMode(hBridgeEnablePin   , OUTPUT);
  pinMode(hBridgeControlPin1 , OUTPUT);
  pinMode(hBridgeControlPin2 , OUTPUT);

  digitalWrite(hBridgeEnablePin   , LOW);
  digitalWrite(hBridgeControlPin1 , LOW);
  digitalWrite(hBridgeControlPin2 , LOW);

  //set up clock manipulating pins
  pinMode(skipOneHourPin   , INPUT);
  pinMode(waitOneHourPin   , INPUT);
  pinMode(skipOneMinutePin , INPUT);
  pinMode(waitOneMinutePin , INPUT);
  buttonPresses.minutesToWait = 0;
  buttonPresses.minutesToSkip = 0;
}

void loop(){
  int currentMinute = getCurrentMinute();
  int currentSecond = getCurrentSecond();
  logWithFlush(String(currentMinute));
  if (currentMinute - 1 == lastSeenMinute ) {
    flipClock(currentMinute);
  }

  lastSeenMinute = currentMinute;
  lastSeenSecond = currentSecond;

  if (!interrupted) {
    sleepForAwhile(currentSecond);
  } else {
    if (!interruptedAtSet) {
      interruptedAtSet = true;
      interruptedAt    = RTC.now();
    }
    if (isOlderThenTenSeconds(interruptedAt, RTC.now())) {
      interrupted      = false;
      interruptedAtSet = false;
    } else {
      //wait for those button presses
      int val;
      val = digitalRead(waitOneHourPin);
      if (val == LOW) {
        buttonPresses.minutesToWait = buttonPresses.minutesToWait + 60;
      }
      val = digitalRead(skipOneHourPin);
      if (val == LOW) {
        buttonPresses.minutesToSkip = buttonPresses.minutesToSkip + 60;
      }
      val = digitalRead(waitOneMinutePin);
      if (val == LOW) {
        buttonPresses.minutesToWait = buttonPresses.minutesToWait + 1;
      }
      val = digitalRead(skipOneMinutePin);
      if (val == LOW) {
        buttonPresses.minutesToWait = buttonPresses.minutesToSkip + 1;
      }
    }
  }
}

bool isOlderThenTenSeconds(DateTime startTime, DateTime currentTime) {
  if(currentTime.unixtime() - startTime.unixtime() > 10) {
    return true;
  } else {
    return false;
  }
}

void registerInterrupt() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();

  if (interrupt_time - last_interrupt_time > 200) {
    Serial.println("I feel interrupted!");
    interrupted = true;
  }
  last_interrupt_time = interrupt_time;
}

void sleepForAwhile(int second) {
  int sleepTime = (int)(1000 * (0.5 * (float) (60 - second)));
  logWithFlush(String(sleepTime));
  sleepTime = max(sleepTime, 1000);
  // overwrite for the time being for testing
  /*sleepTime = 3000;*/
  Sleepy::loseSomeTime(sleepTime);
}

int getCurrentMinute(){
  DateTime now = RTC.now();
  return now.minute();
}

int getCurrentSecond(){
  DateTime now = RTC.now();
  return now.second();
}

void logWithFlush(String msg) {
  Serial.println(msg);
  Serial.flush();
  delay(5);
}

void flipClock(int currentMinute) {
  if (waitsRequired()) {
    buttonPresses.minutesToWait -= 1;
  } else {
    digitalWrite(hBridgeEnablePin, HIGH);

    //could also do both. it doesnt really matter as the coils counteract each other when energized in the wrong way
    if(currentMinute % 2 == 0) {
      digitalWrite(hBridgeControlPin1, HIGH);
      digitalWrite(hBridgeControlPin2, LOW);
    } else {
      digitalWrite(hBridgeControlPin1, LOW);
      digitalWrite(hBridgeControlPin2, HIGH);
    }

    digitalWrite(hBridgeEnablePin, LOW);
  }
}

bool waitsRequired() {
  if (buttonPresses.minutesToWait != 0) {
    return true;
  } else {
    return false;
  }
}
