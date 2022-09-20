#include <Arduino.h>
#include <FastLED.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>
#include <SoftwareSerial.h>

const int RANGE_BOOT_RETRIES = 3;
const int LIGHT_SEN_AMOUNT = 18;
const int LOCK_CONTROL_PIN = 25;   // TBD
const int LOCK_SEN_PIN = 27;       // TBD
const int STOP_BUTTON_PIN = 0;     // TBD
const int START_LED_PIN = 0;       // TBD
const int LED_STRIP_PIN = 0;       // TBD
const int OPERATOR_BUTTON_PIN = 0; // TBD
const int TOUCH_SPEAKER_ADDRESS = 4;
const int ESP_ADDRESS = 0;

const int FALSE_ENTER_RESET_TIME = 5000; // 5 sec
const int HALL_WIDTH = 1100;             // mm
const byte START_SPEAKER_TOUCH = 2;
const byte START_SPEAKER_MAIN = 1;
const byte STOP_SPEAKER = 0;
const int SOUND_DURATION = 1500; // duration of the laser touching sound

bool last_door_sen_val = true;
bool light_sen_val[LIGHT_SEN_AMOUNT];
bool last_light_sen_val[LIGHT_SEN_AMOUNT];
int TOF_val;
bool stop_button_val = false;
bool last_stop_button_val = false;
bool has_finnished = false;
bool last_operator_button_val = false;

//so only the following can be used for RX: 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69)

const int RX_PIN = 0;
const int TX_PIN = 0;

SoftwareSerial screen(RX_PIN, TX_PIN);

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
  // Serial.println("Starting VL53L0X boot");
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
  pinMode(LOCK_SEN_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT);     // PULLUP?
  pinMode(OPERATOR_BUTTON_PIN, INPUT); // PULLUP?
  pinMode(START_LED_PIN, OUTPUT);

  Serial.begin(9600);
  screen.begin(9600);
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
  Wire.beginTransmission(TOUCH_SPEAKER_ADDRESS); // transmit to device #4
  Wire.write(sign);
  Wire.endTransmission();
}

void send_to_screen(int total_time, int touch_count){
  screen.write(total_time);
  screen.write(touch_count);
}

void draw_string(int start_x, int start_y, String str, Panel::Colors color, int size_modifier)
{
  for (int i = 0; i < str.length(); i++)
  {
    panel.drawBigChar(start_x + i * 4, start_y, str[i], color, size_modifier);
  }
}

String curr_time_str;

void loop()
{
  bool door_sen_val = digitalRead(LOCK_SEN_PIN);
  bool has_started = door_sen_val && !last_door_sen_val;
  unsigned long start_time = millis();
  unsigned long hit_time = start_time;
  int touch_counter = 0;
  bool someone_entered = false;

  // Serial.println(door_sen_val);

  digitalWrite(START_LED_PIN, 1); // LED invites to start

  while (has_started)
  {
    send_to_speaker(START_SPEAKER_MAIN); // play mission impossible
    curr_time_str = String((float)(millis() - start_time) / 1000);

    if (lox.isRangeComplete())
    {
      TOF_val = lox.readRange();
      // Serial.print("Distance in mm: ");
      // Serial.println(TOF_val);
    }

    draw_string(4, 0, "your time: ", panel.RED, 1); // shows curr time
    draw_string(4, 8, curr_time_str, panel.RED, 1);

    bool operator_button_val = digitalRead(OPERATOR_BUTTON_PIN);

    if (!operator_button_val && last_operator_button_val)
    { // operator button
      break;
    }

    digitalWrite(LOCK_CONTROL_PIN, 1); // locks door
    digitalWrite(START_LED_PIN, 0);    // LED stops from getting in

    has_finnished = stop_button_val && !last_stop_button_val; // just closed the door
    if (has_finnished)
    {
      break;
    }

    if (TOF_val < HALL_WIDTH - 100)
    { // person's entrance detected
      someone_entered = true;
    }

    if (!someone_entered && millis() - start_time >= FALSE_ENTER_RESET_TIME)
    { // false entrance
      has_started = false;
    }

    for (int i = 2; i < LIGHT_SEN_AMOUNT + 2; i++)
    {
      light_sen_val[i] = digitalRead(i);
      Serial.print(i);
      Serial.print(" : ");
      Serial.print(light_sen_val[i]);
      Serial.print("\t");
      Serial.print(" last: ");
      Serial.print(last_light_sen_val[i]);
      Serial.print("\t");
      Serial.println(!light_sen_val[i] && last_light_sen_val[i]);

      if (!light_sen_val[i] && last_light_sen_val[i])
      {
        touch_counter++;
        hit_time = millis();

        send_to_speaker(START_SPEAKER_TOUCH);
        // Serial.println("speaker on!!");
      }

      last_light_sen_val[i] = light_sen_val[i];
    }

    send_to_screen((int)((millis() - start_time) / 1000), touch_counter);

    // if (millis() - hit_time > SOUND_DURATION)
    // {
    // send_to_speaker(STOP_SPEAKER);
    //   Serial.println("speaker off");
    // }

    last_stop_button_val = stop_button_val;
    last_operator_button_val = operator_button_val;
  }

  send_to_speaker(STOP_SPEAKER);

  digitalWrite(LOCK_CONTROL_PIN, 0); // unlocks door

  last_door_sen_val = door_sen_val;

  float total_time = (millis() - start_time) / 1000; // sec
  String total_time_str = curr_time_str;

  Wire.beginTransmission(ESP_ADDRESS);
  Wire.write((int)total_time);
  Wire.write(touch_counter);
  Wire.endTransmission();
}