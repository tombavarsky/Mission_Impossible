#include <Arduino.h>

int hit_counter = 0;
const int const SENSOR_PINS[0];

void setup()
{
  for (int pin : SENSOR_PINS)
  {
    pinMode(pin, INPUT);
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
}