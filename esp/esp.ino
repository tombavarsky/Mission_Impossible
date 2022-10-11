#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <ArduinoJson.h>

#define STAND_NAME "MissionImpossible" // stand name

int gameResult;
int touch_counter;
int t_time;

const char *host = "iddofroom.wixsite.com";

String url = "/elsewhere/_functions/winner?";

const int httpPort = 443; // 80 for insecure (http), 443 for secure (https). Elsewhere website is https

String text = "";

void read_results(int a)
{
   Serial.print("time is -------------------");
  t_time = Wire.read();
  touch_counter = Wire.read();

  Serial.println(t_time);
}

/*
   ssid - WiFi network name
   password - WiFi network password
   toSerial - true if serial monitor status updates are desired
*/
void ConnectWiFi(char *ssid, char *password, bool toSerial)
{
  if (toSerial)
  {
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (toSerial)
      Serial.print(".");
  }

  if (toSerial)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

/*
   client - server client
   toSerial - true if serial monitor status updates are desired
*/
void ConnectWebsite(WiFiClientSecure client, bool toSerial)
{
  if (toSerial)
  {
    Serial.print("connecting to ");
    Serial.println(host);
  }

  client.setInsecure();

  if (!client.connect(host, httpPort))
  {
    if (toSerial)
      Serial.println("connection failed");
    return;
  }
  if (toSerial)
    Serial.println("connection successful");
}

/*
   client - server client
   result - game score
   toSerial - true if serial monitor status updates are desired
*/
String GETRequest(WiFiClientSecure client, int total_time, int touch_cnt, bool toSerial)
{

  if (toSerial)
  {
    Serial.print("Requesting URL: ");
    Serial.println(url);
  }

  client.print(String("GET ") + url + "stand=" + STAND_NAME + "&time=" + total_time + "&touchCount=" + touch_cnt + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  if (toSerial)
  {
    Serial.println();
    Serial.println("closing connection");
  }

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (toSerial)
      Serial.print("r");
    if (line == "\r")
      Serial.println();
      break;
  }

  while (client.available())
  {
    text += client.readStringUntil('\n');
  }
  if (toSerial)
    Serial.println(text);
  return text;
}
StaticJsonDocument<200> doc;
void setup()
{

  Serial.begin(9600);
  delay(10);
  Wire.begin(2); // address is 2
  Wire.onReceive(read_results);

  ConnectWiFi("Tomba", "tomba123", true);
}

void loop()
{

  WiFiClientSecure client;

  ConnectWebsite(client, false);
  String player_rank = GETRequest(client, t_time, touch_counter, false);

  deserializeJson(doc, player_rank);
  bool did_win = doc["winner"];
  
  Serial.print("WINNER------");
  Serial.println(did_win);

//  Serial.println(player_rank);
  // send rank to arduino
}
