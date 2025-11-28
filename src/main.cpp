// main.cpp
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecureBearSSL.h>

#ifndef STASSID
#define STASSID "TLU"
#define STAPSK ""
#endif

// --- Pin definitions ---
const uint8_t CAR_RED_PIN = 15;
const uint8_t CAR_YELLOW_PIN = 12;
const uint8_t CAR_GREEN_PIN = 13;
const uint8_t PED_RED_PIN = 5;
const uint8_t PED_GREEN_PIN = 2;
const uint8_t NIGHT_MODE_BUTTON_PIN = 14;


// --- Wi-Fi & URLs ---
ESP8266WiFiMulti WiFiMulti;
const String serverTimeURL =
    "https://script.google.com/macros/s/"
    "AKfycbyybt4asK-PLpXC3tB1K4d0FnzCL14ufX9fRWqbaS1bjIF8aWYB8B9E2e126zIRsJnV/"
    "exec";
const String confiURL =
    "https://script.google.com/macros/s/AKfycbyd02AsIlEd3DB_QLDs45rPaTBA7bgFqaB9Br5RF6KapxCo75DBcuBGD06sxo1j3gRb/exec";
const int fooriNr = 2;

// --- State / timing ---
long long nihe = 0; // offset = localMillis - serverMillis
bool serverAegKorras = false;

unsigned long lastConfigPoll = 0;
const unsigned long CONFIG_POLL_INTERVAL = 20000UL; // 20s

unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 300UL; // 300ms

// Config defaults (ms)
unsigned long kestus = 20000UL;
unsigned long foorinihe = 0UL;
bool globalNightModePreference = false;
bool isNightModeActiveLocally = false;

// --- Helper functions ---
String httpsGet(const String &url) {
  if (WiFiMulti.run() != WL_CONNECTED)
    return String();

  std::unique_ptr<BearSSL::WiFiClientSecure> client(
      new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  String payload = "";

  if (https.begin(*client, url)) {
    https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    int httpCode = https.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        payload = https.getString();
      }
    }
    https.end();
  }
  return payload;
}

void sendNightModeToServer(bool state) {
  String url = String(confiURL) + "?ooreziim=" + (state ? "true" : "false");
  String resp = httpsGet(url);
  Serial.printf("[NightMode] sent %s, server replied: %s\n",
                state ? "true" : "false", resp.c_str());
}

// --- Server time fetch ---
void fetchServerTime() {
  Serial.println("[fetchServerTime] fetching server time...");
  String resp = httpsGet(serverTimeURL);
  if (resp.length() == 0) {
    Serial.println("[fetchServerTime] failed to fetch server time");
    return;
  }

  resp.trim();
  long long serverMillis = atoll(resp.c_str());
  unsigned long local = millis();
  nihe = (long long)local - serverMillis;
  serverAegKorras = true;
  Serial.printf("[fetchServerTime] serverMillis=%lld local=%lu nihe=%lld\n",
                serverMillis, local, nihe);
}

// --- Config fetch ---
void fetchConfig() {
  String url = String(confiURL) + "?foorinr=" + String(fooriNr);
  String resp = httpsGet(url);
  if (resp.length() == 0) {
    Serial.println("[fetchConfig] failed to fetch config");
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.print("[fetchConfig] JSON parse error: ");
    Serial.println(err.c_str());
    return;
  }

  if (!doc.is<JsonArray>()) {
    Serial.println("[fetchConfig] config not array");
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() >= 1) {
    long val0 = arr[0].as<long>();
    if (val0 > 0)
      kestus = (unsigned long)val0 * 1000UL;
  }
  if (arr.size() >= 2) {
    long val1 = arr[1].as<long>();
    foorinihe = (unsigned long)val1 * 1000UL;
  }
  if (arr.size() >= 3) {
    globalNightModePreference = arr[2].as<bool>();
  }

  Serial.printf(
      "[fetchConfig] kestus=%lu ms, foorinihe=%lu ms, globalNight=%s\n", kestus,
      foorinihe, globalNightModePreference ? "true" : "false");
}

// --- Server millis ---
unsigned long getServerMillis() {
  long long v = (long long)millis() - nihe;
  return (v < 0) ? 0 : (unsigned long)v;
}

// --- LED helper ---
inline void setPin(uint8_t pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }

// --- Update traffic lights ---
void updateLights() {
  if (!serverAegKorras)
    return;

  unsigned long serverNow = getServerMillis();
  long long temp =
      (long long)serverNow - (long long)kestus + (long long)foorinihe;
  long long tsukliAeg_ll = (kestus == 0) ? 0 : (temp % (long long)kestus);
  if (tsukliAeg_ll < 0)
    tsukliAeg_ll += kestus;
  unsigned long tsukliAeg = (unsigned long)tsukliAeg_ll;

  float t = (float)tsukliAeg / 1000.0f;
  const float redYellow = 1.0f;
  const float greenBlink = 2.0f;
  const float yellow = 1.0f;
  float totalSeconds = (float)kestus / 1000.0f;
  float remaining = totalSeconds - (redYellow + greenBlink + yellow);
  float green = remaining * 0.3f;

  float t1 = redYellow;
  float t2 = t1 + green;
  float t3 = t2 + greenBlink;
  float t4 = t3 + yellow;

  bool currentCarRedOn = (t < t1 || t >= t4);

  // Night mode logic
  if (globalNightModePreference) {
    if (currentCarRedOn)
      isNightModeActiveLocally = true;
  } else {
    if (isNightModeActiveLocally) {
      tsukliAeg = (unsigned long)(t4 * 1000.0f);
      t = (float)tsukliAeg / 1000.0f;
    }
    isNightModeActiveLocally = false;
  }

  // Car lights
  bool redOn = false, yellowOn = false, greenOn = false;
  if (isNightModeActiveLocally) {
    unsigned long now = millis();
    yellowOn = ((now / 500UL) % 2UL) == 0;
  } else {
    float tt = (float)tsukliAeg / 1000.0f;
    if (tt < t1) {
      redOn = true;
      yellowOn = true;
    } else if (tt < t2)
      greenOn = true;
    else if (tt < t3)
      greenOn = (((int)floor((tt - t2) * 2)) % 2) == 0;
    else if (tt < t4)
      yellowOn = true;
    else
      redOn = true;
  }

  // Pedestrian lights
  bool pedRedOn = false, pedGreenOn = false;
  if (!isNightModeActiveLocally) {
    float total = (float)kestus / 1000.0f;
    float jalaStart = fmod((t4 + 1.0f), total);
    float jalaEnd = fmod((t1 - 3.0f + total), total);
    float jt = fmod((float)tsukliAeg / 1000.0f, total);
    bool jalaActive = (jalaStart < jalaEnd) ? (jt >= jalaStart && jt < jalaEnd)
                                            : (jt >= jalaStart || jt < jalaEnd);
    if (jalaActive) {
      float distToEnd = fmod((jalaEnd - jt + total), total);
      if (distToEnd <= 3.0f)
        pedGreenOn = (((int)floor(distToEnd * 2)) % 2) == 0;
      else
        pedGreenOn = true;
      pedRedOn = false;
    } else {
      pedRedOn = true;
      pedGreenOn = false;
    }
  }

  // Apply LEDs
  setPin(CAR_RED_PIN, redOn);
  setPin(CAR_YELLOW_PIN, yellowOn);
  setPin(CAR_GREEN_PIN, greenOn);
  setPin(PED_RED_PIN, pedRedOn);
  setPin(PED_GREEN_PIN, pedGreenOn);

  // Debug
  static unsigned long lastDbg = 0;
  if (millis() - lastDbg > 2000UL) {
    lastDbg = millis();
    Serial.printf("[update] tsukliAeg=%lu red=%d yellow=%d green=%d pedRed=%d "
                  "pedGreen=%d night=%d\n",
                  tsukliAeg, redOn, yellowOn, greenOn, pedRedOn, pedGreenOn,
                  isNightModeActiveLocally ? 1 : 0);
  }
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(CAR_RED_PIN, OUTPUT);
  pinMode(CAR_YELLOW_PIN, OUTPUT);
  pinMode(CAR_GREEN_PIN, OUTPUT);
  pinMode(PED_RED_PIN, OUTPUT);
  pinMode(PED_GREEN_PIN, OUTPUT);
  pinMode(NIGHT_MODE_BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(CAR_RED_PIN, LOW);
  digitalWrite(CAR_YELLOW_PIN, LOW);
  digitalWrite(CAR_GREEN_PIN, LOW);
  digitalWrite(PED_RED_PIN, LOW);
  digitalWrite(PED_GREEN_PIN, LOW);

  Serial.println("=== ESP8266 Traffic Light ===");

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);

  Serial.print("Connecting to WiFi: ");
  Serial.println(STASSID);
  int retries = 30;
  while (WiFiMulti.run() != WL_CONNECTED && retries-- > 0) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected! IP: " + WiFi.localIP().toString());
  else
    Serial.println("Failed to connect to WiFi.");

  fetchServerTime();
  fetchConfig();

  digitalWrite(CAR_GREEN_PIN, HIGH);
  digitalWrite(PED_GREEN_PIN, HIGH);
  delay(1000);
  digitalWrite(CAR_GREEN_PIN, LOW);
  digitalWrite(PED_GREEN_PIN, LOW);
}

// --- Loop ---
void loop() {
  unsigned long now = millis();

   static bool lastButtonState = HIGH;
  bool currentState = digitalRead(NIGHT_MODE_BUTTON_PIN);

  if (currentState == LOW && lastButtonState == HIGH) {
      globalNightModePreference = !globalNightModePreference;

      Serial.println("Vajutati nupp! NightMode toggled -> " +
                     String(globalNightModePreference));

      sendNightModeToServer(globalNightModePreference);

      delay(300);
  }
  lastButtonState = currentState;

  // Config polling
  if (now - lastConfigPoll > CONFIG_POLL_INTERVAL) {
    lastConfigPoll = now;
    fetchConfig();
  }

  // Update lights
  if (now - lastUpdate > UPDATE_INTERVAL) {
    lastUpdate = now;
    updateLights();
  }
 

  delay(2);
}