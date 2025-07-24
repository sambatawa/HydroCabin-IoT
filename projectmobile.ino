#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "time.h"
#include "SD.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "LAPTOP-AIGIFML3 5577"
#define WIFI_PASSWORD "01234567"
#define API_KEY "AIzaSyAv3VXOGJ8FlW0YQmihCP7jOHTN0e4b8do"
#define USER_EMAIL "inassamara07@gmail.com"
#define USER_PASSWORD "00000000"
#define DATABASE_URL "https://hydrocabin-56d6f-default-rtdb.asia-southeast1.firebasedatabase.app/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String uid;
String databasePath;
String parentPath;
int timestamp;
FirebaseJson json;

const char *ntpServer = "pool.ntp.org";

Adafruit_BME280 bme;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000;
const int buzzerPin = 14;
const int ledGreenPin = 12;
const int ledYellowPin = 13;
const int ledRedPin = 15;
const int ledClearPin = 16;

void NotifPin()
{
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledYellowPin, OUTPUT);
  pinMode(ledClearPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledGreenPin, LOW);
  digitalWrite(ledYellowPin, LOW);
  digitalWrite(ledRedPin, LOW);
  digitalWrite(ledClearPin, LOW);
}
void konekBME()
{
  if (!bme.begin(0x76))
  {
    Serial.println("BME tidak terhubung");
    while (1)
      ;
  }
}
void konekWIFI()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println();
  Serial.print("Connected to WiFi. IP: ");
  Serial.println(WiFi.localIP());
}

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Waktu gagal");
    return 0;
  }
  time(&now);
  return now;
}

void setup()
{
  Serial.begin(115200);
  konekBME();
  konekWiFi();
  configTime(0, 0, ntpServer);
  NotifPin();

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);

  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  uid = auth.token.uid.c_str();
  Serial.print("\nUser UID: ");
  Serial.println(uid);
  databasePath = "/SensorData/" + uid + "/readings";
}

void loop()
{
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;
    timestamp = getTime();
    if (hum == 0.00 || temp == 0.00 || pres == 0.00)
    {
      Serial.println("Data tidak valid karena 0.00");
      return;
    }

    int range = 0;
    if (temp < 18 || temp > 25)
      range += 1;
    if (hum < 40 || hum > 60)
      range += 1;
    if (pres < 500 || pres > 1013)
      range += 1;
    Serial.printf("Out of range sensors: %d\n", range);
    switch (range)
    {
    case 0:
      digitalWrite(ledClearPin, HIGH);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledYellowPin, LOW);
      digitalWrite(ledRedPin, LOW);
      noTone(buzzerPin);
      break;
    case 1:
      digitalWrite(ledClearPin, LOW);
      digitalWrite(ledGreenPin, HIGH);
      digitalWrite(ledYellowPin, LOW);
      digitalWrite(ledRedPin, LOW);
      tone(buzzerPin, 2000);
      delay(3000);
      noTone(buzzerPin);
      delay(2000);
      break;
    case 2:
      digitalWrite(ledClearPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledYellowPin, HIGH);
      digitalWrite(ledRedPin, LOW);
      tone(buzzerPin, 2000);
      delay(3000);
      noTone(buzzerPin);
      delay(2000);
      break;
    case 3:
      digitalWrite(ledClearPin, LOW);
      digitalWrite(ledGreenPin, LOW);
      digitalWrite(ledYellowPin, LOW);
      digitalWrite(ledRedPin, HIGH);
      tone(buzzerPin, 2000);
      delay(3000);
      noTone(buzzerPin);
      delay(2000);
      break;
    }
    char formattedTime[25];
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeinfo);
    parentPath = databasePath + "/" + String(timestamp);

    json.clear();
    json.set("/temperature", String(temp));
    json.set("/humidity", String(hum));
    json.set("/pressure", String(pres));
    json.set("/timestamp", String(timestamp));
    json.set("/formatted_time", String(formattedTime));
    Serial.println("Uploading data:");
    Serial.println("Temperature: " + String(temp));
    Serial.println("Humidity: " + String(hum));
    Serial.println("Pressure: " + String(pres));
    Serial.println("Timestamp: " + String(timestamp));
    Serial.println("Time: " + String(formattedTime));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "Success" : fbdo.errorReason().c_str());
  }
  delay(300);
}
