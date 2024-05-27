#include <HCSR04.h>

byte triggerPin = 16;
byte echoPin = 17;

void setup () {
  Serial.begin(115200);
  HCSR04.begin(triggerPin, echoPin);
}

void loop () {
  double* distances = HCSR04.measureDistanceCm();
  
  Serial.print("1: ");
  Serial.print(distances[0]);
  Serial.println(" cm");
  
  Serial.println("---");
  delay(250);
}