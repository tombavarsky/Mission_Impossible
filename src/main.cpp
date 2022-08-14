#include <Arduino.h>
#include <FastLED.h>

const int LIGHT_SEN_AMOUNT = 20;
const int LOCK_CONTROL_PIN = 0;           // TBD
const int LOCK_SEN_PIN = 0;               // TBD
const int STOP_BUTTON_PIN = 0;            // TBD
const int START_LED_PIN = 0;              // TBD
const int MOVMENT_SEN_PIN = 0;            // tbd
const int FALSE_ENTER_RESET_TIME = 10000; // 10 sec

bool last_door_sen_val = true;
bool light_sen_val[20];
bool last_light_sen_val[20];
int TOF_val;
bool stop_button_val = false;
bool last_stop_button_val = false;
bool someone_entered = false;
bool has_finnished = false;

void setup()
{
  pinMode(LOCK_CONTROL_PIN, OUTPUT);
  pinMode(LOCK_SEN_PIN, INPUT);
  pinMode(STOP_BUTTON_PIN, INPUT); // PULLUP?
  pinMode(MOVMENT_SEN_PIN, INPUT);
  pinMode(START_LED_PIN, OUTPUT);

  Serial.begin(9600);

  for (int i = 0; i < LIGHT_SEN_AMOUNT; i++)
  {
    pinMode(i, INPUT);
  }
}

void loop()
{
  bool door_sen_val = digitalRead(LOCK_SEN_PIN);
  bool has_started = door_sen_val && !last_door_sen_val;
  long start_time = millis();
  int touch_counter = 0;

  while (has_started)
  {
    has_finnished = stop_button_val && !last_stop_button_val;
    if (has_finnished)
    {
      break;
    }

    if (millis() - start_time >= FALSE_ENTER_RESET_TIME && !someone_entered)
    { // TODO: set someone_entered using the sensor
      has_started = false;
    }

    for (int i = 0; i < LIGHT_SEN_AMOUNT; i++)
    {
      light_sen_val[i] = digitalRead(i);

      if (!light_sen_val[i] && last_light_sen_val[i])
      {
        touch_counter++;
      }
      last_light_sen_val[i] = light_sen_val[i];
    }

    last_stop_button_val = stop_button_val;
  }

  last_door_sen_val = door_sen_val;

  long total_time = millis() - start_time;
}