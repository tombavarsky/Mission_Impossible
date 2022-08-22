#include <Arduino.h>
#include <FastLED.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>
#include <Panel.h>

const int RANGE_BOOT_RETRIES = 3;
const int LIGHT_SEN_AMOUNT = 18;
const int LOCK_CONTROL_PIN = 0;    // TBD
const int LOCK_SEN_PIN = 0;        // TBD
const int STOP_BUTTON_PIN = 0;     // TBD
const int START_LED_PIN = 0;       // TBD
const int MOVMENT_SEN_PIN = 0;     // TBD
const int OPERATOR_BUTTON_PIN = 0; // TBD

const int FALSE_ENTER_RESET_TIME = 10000; // 10 sec
const int HALL_WIDTH = 1100;              // mm
const byte START_SPEAKER_SIGN = 1;
const byte STOP_SPEAKER_SIGN = 0;
const int SOUND_DURATION = 2000; // duration of the laser touching sound

bool last_door_sen_val = true;
bool light_sen_val[LIGHT_SEN_AMOUNT];
bool last_light_sen_val[LIGHT_SEN_AMOUNT];
int TOF_val;
bool stop_button_val = false;
bool last_stop_button_val = false;
bool has_finnished = false;
bool last_operator_button_val = false;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Panel panel(32, 64);

bool rangeBooted = false;
int rangeBootCnt = 0;
int range = -1;
int rangeAccum = -1;
// bool rangeActive = false;
int rangeCnt = 0;

void lox_setup()
{
  Serial.println("Starting VL53L0X boot");
  while ((!rangeBooted) && (rangeBootCnt < RANGE_BOOT_RETRIES))
  {
    if (lox.begin())
    {
      Serial.print(F("VL53L0X sensor boot done successfully after "));
      Serial.print(rangeBootCnt);
      Serial.println(" attempts.");
      rangeBooted = true;
    }
    else
    {
      Serial.print(F("Failed to boot VL53L0X, retrying.. "));
      Serial.println(rangeBootCnt);
      rangeBootCnt++;
      delay(1000);
    }
  }
  if (rangeBooted)
  {
    if (!lox.startRangeContinuous())
    {
      Serial.println(F("Failed to start VL53L0X continuous ranging\n"));
    }
    else
    {
      Serial.println(F("VL53L0X sensor started in continuous ranging mode.\n"));
    }
  }
  else
  {
    Serial.println(F("Failed to boot VL53L0X, continuing without range sensor, restart teensy to retry."));
  }
}

void setup()
{
  pinMode(LOCK_CONTROL_PIN, OUTPUT);
  pinMode(LOCK_SEN_PIN, INPUT);
  pinMode(STOP_BUTTON_PIN, INPUT);     // PULLUP?
  pinMode(OPERATOR_BUTTON_PIN, INPUT); // PULLUP?
  pinMode(MOVMENT_SEN_PIN, INPUT);
  pinMode(START_LED_PIN, OUTPUT);

  Serial.begin(9600);
  Wire.begin();
  panel.createBufferBG(panel.BLACK); // background black

  for (int i = 2; i < LIGHT_SEN_AMOUNT + 2; i++)
  {
    pinMode(i, INPUT);
  }

  lox_setup();
}

void send_to_speaker(byte sign)
{
  Wire.write("sign is "); // sends five bytes to speaker
  Wire.write(sign);       // sends one byte to speaker
}

void draw_string(int start_x, int start_y, String str, Panel::Colors color, int size_modifier)
{
  for (int i = 0; i < str.length(); i++)
  {
    panel.drawBigChar(start_x + i * 4, start_y, str[i], color, size_modifier);
  }
}

void loop()
{
  bool door_sen_val = digitalRead(LOCK_SEN_PIN);
  bool has_started = door_sen_val && !last_door_sen_val;
  long long start_time = millis();
  long long hit_time = start_time;
  int touch_counter = 0;
  bool someone_entered = false;

  Wire.beginTransmission(4); // transmit to device #4

  if (lox.isRangeComplete())
  {
    Serial.print("Distance in mm: ");
    TOF_val = lox.readRange();
    Serial.println(TOF_val);
  }

  digitalWrite(START_LED_PIN, 1);

  while (has_started)
  {
    bool operator_button_val = digitalRead(OPERATOR_BUTTON_PIN);

    if (!operator_button_val && last_operator_button_val)
    {
      break;
    }

    digitalWrite(LOCK_CONTROL_PIN, 1); // locks door
    digitalWrite(START_LED_PIN, 0);

    has_finnished = stop_button_val && !last_stop_button_val; // just closed the door
    if (has_finnished)
    {
      break;
    }

    if (TOF_val < HALL_WIDTH - 100)
    {
      someone_entered = true;
    }

    if (!someone_entered && millis() - start_time >= FALSE_ENTER_RESET_TIME)
    { // TODO: set someone_entered using the sensor
      has_started = false;
    }

    for (int i = 0; i < LIGHT_SEN_AMOUNT; i++)
    {
      light_sen_val[i] = digitalRead(i);

      if (!light_sen_val[i] && last_light_sen_val[i])
      {
        touch_counter++;
        hit_time = millis();

        send_to_speaker(START_SPEAKER_SIGN);
      }

      last_light_sen_val[i] = light_sen_val[i];
    }

    if (millis() - hit_time > SOUND_DURATION)
    {
      send_to_speaker(STOP_SPEAKER_SIGN);
    }

    last_stop_button_val = stop_button_val;
    last_operator_button_val = operator_button_val;
  }

  send_to_speaker(STOP_SPEAKER_SIGN);

  digitalWrite(LOCK_CONTROL_PIN, 0); // unlocks door

  last_door_sen_val = door_sen_val;

  float total_time = (millis() - start_time) / 1000; // sec
  String total_time_str = String(total_time);

  // panel part
  draw_string(4, 0, "your time: ", panel.RED, 1);

  // panel.drawBigChar(4, 0, 'y', panel.RED, 1);
  // panel.drawBigChar(8, 0, 'o', panel.RED, 1);
  // panel.drawBigChar(12, 0, 'u', panel.RED, 1);
  // panel.drawBigChar(16, 0, 'r', panel.RED, 1);
  // panel.drawBigChar(20, 0, ' ', panel.RED, 1);
  // panel.drawBigChar(24, 0, 't', panel.RED, 1);
  // panel.drawBigChar(28, 0, 'i', panel.RED, 1);
  // panel.drawBigChar(32, 0, 'm', panel.RED, 1);
  // panel.drawBigChar(36, 0, 'e', panel.RED, 1);
  // panel.drawBigChar(40, 0, ':', panel.RED, 1);
  // panel.drawBigChar(40, 0, ' ', panel.RED, 1);

  draw_string(4, 8, total_time_str, panel.RED, 1);

  // for (int i = 0; i < total_time_str.length(); i++)
  // {
  //   panel.drawBigChar((i * 4) + 4, 8, total_time_str[i], panel.RED, 1);
  // }

  draw_string(4, 16, "number of", panel.RED, 1);
  draw_string(4, 20, "hits: ", panel.RED, 1);
  draw_string(28, 20, String(touch_counter), panel.RED, 1);

  // panel.drawBigChar(4, 16, 'n', panel.RED, 1);
  // panel.drawBigChar(8, 16, 'u', panel.RED, 1);
  // panel.drawBigChar(12, 16, 'm', panel.RED, 1);
  // panel.drawBigChar(16, 16, 'b', panel.RED, 1);
  // panel.drawBigChar(20, 16, 'e', panel.RED, 1);
  // panel.drawBigChar(24, 16, 'r', panel.RED, 1);
  // panel.drawBigChar(28, 16, ' ', panel.RED, 1);
  // panel.drawBigChar(32, 16, 'o', panel.RED, 1);
  // panel.drawBigChar(36, 16, 'f', panel.RED, 1);
  // panel.drawBigChar(4, 20, 'h', panel.RED, 1);
  // panel.drawBigChar(8, 20, 'i', panel.RED, 1);
  // panel.drawBigChar(12, 20, 't', panel.RED, 1);
  // panel.drawBigChar(16, 20, 's', panel.RED, 1);
  // panel.drawBigChar(20, 20, ':', panel.RED, 1);
  // panel.drawBigChar(24, 20, ' ', panel.RED, 1);
  // panel.drawBigChar(28, 20, touch_counter, panel.RED, 1);
}