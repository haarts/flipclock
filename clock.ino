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

#define controlPin1 8
#define controlPin2 9
#define enablePin 10
#define interruptPin 2

void setup() {
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  
  if( !RTC.isrunning()){
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  attachInterrupt(0, registerInterrupt, FALLING);

  pinMode(enablePin, OUTPUT);
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);
  pinMode(controlPin2, INPUT);

  digitalWrite(enablePin, LOW);
  digitalWrite(controlPin1, LOW);
  digitalWrite(controlPin2, LOW);

  lastSeenMinute = getCurrentMinute();
  lastSeenSecond = getCurrentSecond();
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
    interrupted = false;
    //can't read the RTC in the interrupt handler. This will hang
    interruptedAt = RTC.now();
    sleepForAwhile(currentSecond);
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
  digitalWrite(enablePin, HIGH);

  //could also do both. it doesnt really matter as the coils counteract each other when energized in the wrong way
  if(currentMinute % 2 == 0) {
    digitalWrite(controlPin1, HIGH);
    digitalWrite(controlPin2, LOW);
  } else {
    digitalWrite(controlPin1, LOW);
    digitalWrite(controlPin2, HIGH);
  }

  digitalWrite(enablePin, LOW);
}
