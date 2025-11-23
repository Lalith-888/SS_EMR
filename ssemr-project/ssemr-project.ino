
#include "pswd.env.h"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <BlynkSimpleEsp8266.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------- PIN MAPPING --------
#define PIN_DHT        D6
#define PIN_PIR        D7
#define PIN_FLAME      D8
#define PIN_IR1        D3
#define PIN_IR2        D4
#define PIN_MQ         A0
#define PIN_BUZZER     D0
#define PIN_SERVO      D5  // FAN Servo
// LCD SDA=D2, SCL=D1

// -------- CONSTANTS --------
const float TEMP_THRESHOLD = 30.0;
const int   AQI_THRESHOLD  = 400;

const int FAN_OFF_ANGLE = 0;
const int FAN_ON_ANGLE  = 120;

DHT dht(PIN_DHT, DHT11);
Servo fanServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
BlynkTimer timer;

// STATE
bool device_fan = false;
bool device_buzzer = false;
bool auto_mode = true;
int occupancy = 0;

bool lastIR1 = LOW, lastIR2 = LOW;
unsigned long lastMotionTime = 0;

// -------- FUNCTION DECLARATIONS --------
void handleStatusHTTP();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void pushStatusToWS();
void checkSensorsAndAct();
void updateLCD(float t, float h, int aqi, bool flame);

int runTinyMLInference(float temp, float hum, int aqi, bool motion);

String webpage;

void fanOn(){
  device_fan = true;
  fanServo.write(FAN_ON_ANGLE);
  Blynk.virtualWrite(V11, 1);
}

void fanOff(){
  device_fan = false;
  fanServo.write(FAN_OFF_ANGLE);
  Blynk.virtualWrite(V11, 0);
}

void setup() {
  Serial.begin(115200);
  delay(50);

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_FLAME, INPUT);
  pinMode(PIN_IR1, INPUT);
  pinMode(PIN_IR2, INPUT);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  dht.begin();

  fanServo.attach(PIN_SERVO);
  fanServo.write(FAN_OFF_ANGLE);

  // LCD INIT
  Wire.begin(D2, D1);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("SS-EMR Booting...");

  // BUILD LOCAL DASHBOARD PAGE
  webpage = "<html><body><h3>SS-EMR Local Panel</h3>Device Running</body></html>";

  // WIFI
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting");
  while(WiFi.status() != WL_CONNECTED){ delay(300); Serial.print("."); }

  Serial.println("\nWiFi Connected!");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("IP:");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());

  // HTTP
  server.on("/", [](){ server.send(200, "text/html", webpage); });
  server.begin();

  // Websocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

  timer.setInterval(1000L, pushStatusToWS);
  timer.setInterval(1000L, checkSensorsAndAct);
}

void loop(){
  server.handleClient();
  webSocket.loop();
  timer.run();

  // -------- IR OCCUPANCY COUNT --------
  bool ir1 = digitalRead(PIN_IR1);
  bool ir2 = digitalRead(PIN_IR2);

  if(ir1 && !lastIR1){ occupancy++; delay(30); }
  if(ir2 && !lastIR2 && occupancy > 0){ occupancy--; delay(30); }

  lastIR1 = ir1;
  lastIR2 = ir2;
}


// -------- HTTP STATUS ENDPOINT --------
void handleStatusHTTP(){
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int   a = analogRead(PIN_MQ);
  bool motion = digitalRead(PIN_PIR);
  bool flame  = digitalRead(PIN_FLAME);

  StaticJsonDocument<256> doc;
  doc["temp"] = t;
  doc["hum"]  = h;
  doc["aqi"]  = a;
  doc["motion"] = motion;
  doc["flame"] = flame;
  doc["occ"]   = occupancy;

  String out;  
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}


// -------- WEBSOCKET EVENTS --------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len){
  if(type != WStype_TEXT) return;

  DynamicJsonDocument doc(200);
  deserializeJson(doc, payload);

  String dev = doc["dev"];
  String act = doc["action"];

  if(dev == "fan"){
    if(act == "on") fanOn();
    else fanOff();
  }
  if(dev == "buzzer"){
    digitalWrite(PIN_BUZZER, act=="on"?HIGH:LOW);
  }
  if(dev == "auto"){
    auto_mode = !auto_mode;
    Blynk.virtualWrite(V30, auto_mode?1:0);
  }
}


// -------- PUSH STATUS TO WS + BLYNK --------
void pushStatusToWS(){

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  int a = analogRead(PIN_MQ);
  bool motion = digitalRead(PIN_PIR);
  bool flame  = digitalRead(PIN_FLAME);

  // BLYNK UPDATE
  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, a);
  Blynk.virtualWrite(V3, occupancy);
  Blynk.virtualWrite(V4, motion);
  Blynk.virtualWrite(V5, flame);
  Blynk.virtualWrite(V11, device_fan);

  // TinyML
  if(runTinyMLInference(t, h, a, motion) == 1){
    Blynk.logEvent("tinyml_alert", "🔥 TinyML Alert detected!");
    digitalWrite(PIN_BUZZER, HIGH);
  }

  updateLCD(t, h, a, flame);

  // WEBSOCKET JSON
  DynamicJsonDocument doc(256);
  doc["temp"] = t;
  doc["hum"]  = h;
  doc["aqi"]  = a;
  doc["occ"]  = occupancy;
  doc["motion"] = motion;
  doc["flame"] = flame;
  doc["auto"] = auto_mode;

  String out;
  serializeJson(doc, out);
  webSocket.broadcastTXT(out);
}


// -------- AUTO CONTROL SYSTEM --------
void checkSensorsAndAct(){

  if(!auto_mode) return;

  float t = dht.readTemperature();
  int a = analogRead(PIN_MQ);
  bool flame = digitalRead(PIN_FLAME);

  // FAN AUTO CONTROL
  if(t >= TEMP_THRESHOLD || a >= AQI_THRESHOLD){
    fanOn();
  } else {
    fanOff();
  }

  // FLAME EMERGENCY
  if(flame){
    Blynk.logEvent("flame_detected", "🔥 Flame detected!");
    digitalWrite(PIN_BUZZER, HIGH);
  }
}


// -------- TINYML DUMMY --------
int runTinyMLInference(float temp, float hum, int aqi, bool motion){
  if(digitalRead(PIN_FLAME)) return 1;
  if(aqi > 900) return 1;
  if(temp > 80) return 1;
  return 0;
}


// -------- LCD UPDATE --------
void updateLCD(float t, float h, int aqi, bool flame){
  static unsigned long last = 0;
  if(millis() - last < 1500) return;
  last = millis();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:");
  lcd.print(t,1);
  lcd.print(" H:");
  lcd.print(h,0);

  lcd.setCursor(0,1);
  lcd.print("AQ:");
  lcd.print(aqi);
  lcd.print(" Fan:");
  lcd.print(device_fan?"ON ":"OFF");
}
