#include <Arduino.h>
#include <FastLED.h>
#include <Adafruit_VL53L0X.h>
#include <Wire.h>
#include <SoftwareSerial.h>

const int NUM_LEDS = 20;
const int LIGHT_SEN_AMOUNT = 10;
const int LOCK_CONTROL_PIN = 25;   // TBD
const int LOCK_SEN_PIN = 12;
const int STOP_BUTTON_PIN = 17;
const int START_LED_PIN = 0;       // TBD
const int LED_STRIP_PIN = 13;
const int OPERATOR_BUTTON_PIN = 0; // TBD
const int TOUCH_SPEAKER_ADDRESS = 4;
const int ESP_ADDRESS = 2; //tbd
const int SCREEN_ADDRESS = 1; //tbd

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
long screen_update_time = 0;
byte last_sign_sent = 3;


CRGB leds[NUM_LEDS];


void setup()
{
  FastLED.addLeds<NEOPIXEL, LED_STRIP_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed

  pinMode(LOCK_CONTROL_PIN, OUTPUT);
  pinMode(LOCK_SEN_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);     // PULLUP?
  pinMode(OPERATOR_BUTTON_PIN, INPUT); // PULLUP?
  pinMode(START_LED_PIN, OUTPUT);

  Serial.begin(9600);
  // screen.begin(9600);
  Wire.begin();

  for (int i = 2; i < LIGHT_SEN_AMOUNT + 2; i++)
  {
    pinMode(i, INPUT);
  }

  // lox_setup();
}

void send_to_speaker(byte sign)
{
  Wire.beginTransmission(TOUCH_SPEAKER_ADDRESS); // transmit to device #4
  Wire.write(sign);
  Wire.endTransmission();
  Serial.print("sent sign --------------  ");
  Serial.println(sign);
}

void send_to_screen(int total_time, int touch_count){
  Wire.beginTransmission(SCREEN_ADDRESS);
  Wire.write(total_time);
  Wire.write(touch_count);
  Wire.endTransmission();
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
  bool first_run = true;

  // Serial.println("start");

  digitalWrite(START_LED_PIN, 1); // LED invites to start

  last_door_sen_val = door_sen_val;

  while (has_started)
  {
    door_sen_val = digitalRead(LOCK_SEN_PIN);
    has_finnished = false;

    if(door_sen_val && !last_door_sen_val){
      break;
    }
    
    last_door_sen_val = door_sen_val;

    if(first_run){
      send_to_speaker(START_SPEAKER_MAIN); // play mission impossible
    }
    first_run = false;

    curr_time_str = String((float)(millis() - start_time) / 1000);

    bool operator_button_val = digitalRead(OPERATOR_BUTTON_PIN);
    last_stop_button_val = stop_button_val;
    stop_button_val = digitalRead(STOP_BUTTON_PIN);

    digitalWrite(LOCK_CONTROL_PIN, 1); // locks door
    digitalWrite(START_LED_PIN, 0);    // LED stops from getting in

    has_finnished = stop_button_val && !last_stop_button_val; // just closed the door

    if (!stop_button_val)
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
      // delay(50);
      // if (i == 7){
      light_sen_val[i] = digitalRead(i);
      Serial.print(i);
      // Serial.print(" : ");
      // Serial.print(light_sen_val[i]);
      // Serial.print("\t");
      // Serial.print(" last: ");
      // Serial.print(last_light_sen_val[i]);
      // Serial.print("\t");
      // Serial.println(!light_sen_val[i] && last_light_sen_val[i]);
      // }

      if (!light_sen_val[i] && last_light_sen_val[i])
      {
        touch_counter++;
        hit_time = millis();

        for(int i = 0; i < NUM_LEDS; i++){
          leds[i] = CRGB::Red;
        }
        FastLED.show();

        send_to_speaker(START_SPEAKER_TOUCH);
        // Serial.println("speaker on!!");
      }

      last_light_sen_val[i] = light_sen_val[i];
    }
    Serial.println();
    long curr_time_milisec = millis() - start_time;

    if (curr_time_milisec - screen_update_time >= 1000){
      Serial.println(touch_counter);
      screen_update_time = curr_time_milisec;
      send_to_screen(int(curr_time_milisec / 1000), touch_counter);
    }

    // if (millis() - hit_time > SOUND_DURATION)
    // {
    // send_to_speaker(STOP_SPEAKER);
    //   Serial.println("speaker off");
    // }

    
    last_operator_button_val = operator_button_val;
  }

  for(int i = 0; i < NUM_LEDS; i++){
    leds[i] = CRGB::Black;
  }

  FastLED.show(); 

  screen_update_time = 0;

  if(!first_run){
    send_to_speaker(STOP_SPEAKER);
  }
  
  digitalWrite(LOCK_CONTROL_PIN, 0); // unlocks door

  float total_time = (millis() - start_time) / 1000; // sec
  String total_time_str = curr_time_str;
  if(has_finnished){
    Wire.beginTransmission(ESP_ADDRESS);
    Wire.write((int)total_time);
    Wire.write(touch_counter);
    Wire.endTransmission();
  }
}