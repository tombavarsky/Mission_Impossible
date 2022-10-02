#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>

#define STAND_NAME "MissionImpossible" // stand name

int gameResult;
int touch_count;
int time;

const char *host = "iddofroom.wixsite.com";

String url = "/elsewhere/_functions/winner?";

const int httpPort = 443; // 80 for insecure (http), 443 for secure (https). Elsewhere website is https

String text = "";

void setup()
{
  Serial.begin(115200);
  delay(10);
  Wire.begin(2); // address is 2
  Wire.onReceive(read_results);

  ConnectWiFi("usrname", "pswrd", true);
}

void loop()
{

  WiFiClientSecure client;

  ConnectWebsite(client, true);
  String player_rank = GETRequest(client, time, touch_count, true);

  Serial.println(player_rank);
  // send rank to arduino
}

void read_results()
{
  time = Wire.read();
  touch_counter = Wire.read();
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
      Serial.println("reading");
    if (line == "\r")
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
