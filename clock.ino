#include <Wire.h>
#include "RTClib.h"
#include "Ports.h"

RTC_DS1307 RTC;

/*EMPTY_INTERRUPT(WDT_vect);*/
ISR(WDT_vect) { Sleepy::watchdogEvent(); }

int lastSeenMinute;
#define controlPin1 8
#define controlPin2 9
#define enablePin 10

void setup() {
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();
  
  if( !RTC.isrunning()){
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  pinMode(enablePin, OUTPUT);
  pinMode(controlPin1, OUTPUT);
  pinMode(controlPin2, OUTPUT);

  digitalWrite(enablePin, LOW);
  digitalWrite(controlPin1, LOW);
  digitalWrite(controlPin2, LOW);

  lastSeenMinute = getCurrentMinute();
}

void loop(){
  int currentMinute = getCurrentMinute();
  logWithFlush(String(currentMinute));
  if (currentMinute - 1 == lastSeenMinute ) {
    flipClock(currentMinute);
  }
  lastSeenMinute = currentMinute;
  Sleepy::loseSomeTime(1000);
}

int getCurrentMinute(){
  DateTime now = RTC.now();
  return now.minute();
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
    digitalWrite(controlPin1, HIGH)
    digitalWrite(controlPin2, LOW)
  } else {
    digitalWrite(controlPin1, LOW)
    digitalWrite(controlPin2, HIGH)
  }

  digitalWrite(enablePin, LOW);
}
