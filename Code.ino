#if defined(ARDUINO_spresense_ast)

#include <Arduino.h>
#include <GNSS.h>
static SpGnss Gnss;
SpNavData ND;
unsigned long TimeStamp;

void setup() {
  //  Serial.begin(115200); // USB
  Serial2.begin(115200); // UART
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SCL, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  // Start GPS
  Gnss.begin();
  // USA
  Gnss.select(GPS);
  Gnss.start(COLD_START);
  digitalWrite(LED_BUILTIN, 0);
  // Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
  // >username,password,clientID
  Serial2.print(">8a4e1f40-a7bb-11ea-a67f-15e30d90bbf4,5d560678b6bf95417b5b789f794f7b24b75e2776,378ff9b0-f1e5-11ec-8da3-474359af83d7");
}

void loop() {
  // Send every 20 seconds
  if (millis() - TimeStamp > 20000)
  {
    TimeStamp = millis();
    Gnss.getNavData(&ND);
    Serial2.print("|");
    // Use only valid data to end comma separated data starting with "|"// Convert m to feet & m/s to mph
    if (ND.posDataExist != 0) Serial2.print(String(ND.latitude, 5) + "," + String(ND.longitude, 5) + "," + String(int(ND.altitude * 3.28084)) + "," + String(int(ND.velocity * 2.23694)));
  }
  if (Serial2.available()) {
    String TempStr = Serial2.readString();
    if (TempStr.equals("1")) digitalWrite(SCL, 0);
    if (TempStr.equals("0")) digitalWrite(SCL, 1);
  }
}

#elif defined(ARDUINO_ESP8266_WEMOS_D1MINI)

#include <Arduino.h>
#include <CayenneMQTTESP8266.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#define StatusUpdate Cayenne.virtualWrite(2, !LEDStauts, "digital_sensor", "d");
char username[50], password[50], clientID[50];
String UARTText;
bool LEDStauts;

String UARTTextPart(int P) {
  int Index = 0;
  for (int i = 1; i < P; i++)Index = UARTText.indexOf(',', Index + 1);
  return UARTText.substring(Index + 1, UARTText.indexOf(',', Index + 1));
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);
  while (!UARTText.startsWith(">")) UARTText = Serial.readString();
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  //wm.resetSettings();//reset settings - wipe credentials for testing
  wm.autoConnect("WeMos");
  strcpy(username, String(UARTTextPart(1)).c_str());
  strcpy(password, String(UARTTextPart(2)).c_str());
  strcpy(clientID, String(UARTTextPart(3)).c_str());
  Cayenne.begin(username, password, clientID);
  digitalWrite(LED_BUILTIN, 1);
  StatusUpdate
}

void loop() {
  Cayenne.loop();
  if (Serial.available()) {
    UARTText = Serial.readString();
    // We have text
    if (UARTText.startsWith("|")) {
      if (UARTText.length() > 1) {
        Cayenne.virtualWrite(3, String(UARTTextPart(1) + "," + UARTTextPart(2) + "," + UARTTextPart(3)).c_str(), "gps", "m");
        Cayenne.virtualWrite(4, UARTTextPart(4).toInt());
      }
      StatusUpdate
    }
  }
}

// Function for processing actuator commands from the Cayenne Dashboard.
CAYENNE_IN(1)
{
  // Write the value received
  LEDStauts = getValue.asInt();
  Serial.print(LEDStauts);
  StatusUpdate
}

#else
#error Unsupported board selection.
#endif
