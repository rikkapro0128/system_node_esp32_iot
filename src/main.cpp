#include <Macro.h>

#include <esp_task_wdt.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <addons/RTDBHelper.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>

#ifdef COLOR
#include <Adafruit_NeoPixel.h>
#endif

#include <MiruEEPROM.h>

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

typedef enum
{
  SSID_TOO_LONG = 0,
  PASSWORD_TOO_LONG = 1,
  CONFIG_NOT_FOUND = 2,
  CONFIG_DONE = 3,
} status_config_wifi;

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* MACRO - NODE TYPE - LOGIC *********************************************************************************************************************************************************************************************************************************
 */

#ifdef LOGIC

#define MAX_NAME_INDEX_FIREBASE 25
#define TIMER_INTERVAL_MS 1000
#define ERROR_TIMER 10
#define PIN_OUT 3
#define NUMS_TIMER 30

#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* MACRO - NODE TYPE - COLOR *********************************************************************************************************************************************************************************************************************************
 */

#ifdef COLOR

// define here...

typedef enum MODE_RGB
{
  SINGLE = 0,
  AUTO = 1,
};

typedef enum INDEX_COLOR
{
  COLOR_R = 0,
  COLOR_G = 1,
  COLOR_B = 2,
  COLOR_ALPHA = 3,
};

#define DI GPIO_NUM_27
#define NUMS_LED 0

#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* VARIABLE GLOBAL *********************************************************************************************************************************************************************************************************************************
 */

// => STATIC
static const char *ntpServer = "pool.ntp.org";
static const char *CHost = "miru.io-";

// => RAM INSIDE
String getMac = WiFi.macAddress();
String GEN_ID_BY_MAC = String(getMac);
String ID_DEVICE;
String DATABASE_URL = "";
unsigned long timeStartResetPinDetach;
unsigned long epochTime;
unsigned long wifiStatus;
unsigned long poolingReconnectWifi;
unsigned long poolingCheckStream;
unsigned long blinkStart;
unsigned long timeIntervalCheckRamSize;
uint8_t countBlink = 0;
uint8_t countConnectionFailure = 0;
bool restartConfigWifi = false;
bool lostConnection = false;
bool blockBtnReset = false;
bool disableBtnReset = false;
bool globalRestart = false;
bool lostStream = false;
bool disconnected = false;
size_t timeoutWifi = 0;
bool wifiIsConnected = false;
bool requestConfigWifi = false;
bool duringConfigWifi = false;

IPAddress ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

struct
{
  String ssid;
  String password;
} WifiConfig;

// => RAM NODE TYPE LOGIC
#ifdef LOGIC

static const char *TYPE_DEVICE = "LOGIC";
bool timeControll = false;
bool blockControl = false;
bool control = true;
bool isFirst = true;
size_t timeDebounceLocalControl = 0;
size_t idControl = 0;
int uartPicControl = 0;
unsigned long sendDataPrevMillis = 0;
unsigned long timerStack[NUMS_TIMER][3];
bool stateDevice[3] = {false, false, false};
bool stateDeviceFirebase[3] = {false, false, false};
char indexTimerStack[NUMS_TIMER][MAX_NAME_INDEX_FIREBASE];

// => Firebase data
FirebaseJson timerParseArray;
FirebaseJsonData timerJson;
FirebaseJsonData deviceJson;

#endif

#ifdef COLOR
// define here...
static const char *TYPE_DEVICE = "COLOR";

uint8_t colorPresentState[4] = {0, 0, 255, 0}; // => rgb(0 -> 255, 0 -> 255, 0 -> 255, 0 -> 100)
uint8_t colorFultureState[4] = {0, 0, 255, 0}; // => rgb(0 -> 255, 0 -> 255, 0 -> 255, 0 -> 100)
int numLedPresent = 0;
bool acceptChange = false;
bool clearEffect = false;
bool requestRestart = false;
unsigned long rgbModeAuto;
uint16_t autoIndexI = 0, autoIndexJ = 0;

// => Firebase data
FirebaseJsonData deviceJson;

#endif

// => Firebase data general
FirebaseJson jsonNewDevice;

// => JSON DOCUMENT
DynamicJsonDocument bufferBodyPaserModeAP(400);

/**
 * ********* CONTRUCTOR NODE TYPE - COLOR *********************************************************************************************************************************************************************************************************************************
 */

#ifdef COLOR
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMS_LED, DI, NEO_GRB + NEO_KHZ800);
#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* PROTOTYPE GENERAL *********************************************************************************************************************************************************************************************************************************
 */

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

// => DEBUG FUNC

#ifdef DEBUG
void viewEEPROM();
#endif

// => FUNC WEBSEVER SERVER
void controllDevices(AsyncWebServerRequest *request, JsonVariant &json);
void pingResponse(AsyncWebServerRequest *request);
void checkLinkAppication(AsyncWebServerRequest *request);
void linkAppication(AsyncWebServerRequest *request, JsonVariant &json);
void addConfiguration(AsyncWebServerRequest *request, JsonVariant &json);
void restartWifiAccess(AsyncWebServerRequest *request);
void resetConfigurationWifi(AsyncWebServerRequest *request);
void resetConfigurationFirebase(AsyncWebServerRequest *request);
void checkConfiguration(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);
void restartEsp(AsyncWebServerRequest *request);

// => FUNC EXECUTE GENERAL

status_config_wifi connectWifiByConfig(String ssid, String password = "");
wl_status_t waitConnectWifi();
void initWatchdogTimer();
bool removeNodeEsp();
void initSignal();
void initResetDeflaut();
void checkRam();
uint32_t ramHeapSize();
void setupStreamFirebase();
void checkFirebaseInit();
void setupWifiModeStation();
void setupWebserverModeAP();
void clearBufferFirebaseDataAll();
unsigned long Get_Epoch_Time();
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
unsigned int strLength(const char *str);

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* PROTOTYPE NODE TYPE -> LOGIC *********************************************************************************************************************************************************************************************************************************
 */

#ifdef LOGIC
#ifdef DEBUG
void PrintListTimer();
#endif
void parserTimerJson(FirebaseStream &data, uint8_t numberDevice, bool isInit = true);
void parserDeviceStatus(FirebaseStream &data, uint8_t numberDevice);
void controllDevice(uint8_t numDevice, bool state, bool syncFirebase = false, bool ignore = false);
void readTimer(FirebaseJson &fbJson, uint8_t numberDevice, String keyAdd = "");
void removeTimer(unsigned long stack[][3], char stackName[][MAX_NAME_INDEX_FIREBASE], String key, bool isCallBack = false);
void sortTimer(unsigned long stack[][3], char stackName[][MAX_NAME_INDEX_FIREBASE]);
#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* PROTOTYPE NODE TYPE -> COLOR *********************************************************************************************************************************************************************************************************************************
 */

// define here....
#ifdef COLOR

bool fadeColor(INDEX_COLOR index);
void setColorChainRGBA(uint32_t c, uint8_t a);
void parserColorRGB(FirebaseStream &data, String prefix = "");
uint8_t parserColorMode(FirebaseStream &data, bool init = false);
int parserNumsLed(FirebaseStream &data, bool init = false);
uint32_t Wheel(byte WheelPos);
void clearEffectColor(uint8_t brightness = 100);
void clearColor();

#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 * ********* EEPROM *********************************************************************************************************************************************************************************************************************************
 */
EepromMiru eeprom(350, "esp8266-device-db-default-rtdb.firebaseio.com");

/**
 * ********* SERVER *********************************************************************************************************************************************************************************************************************************
 */
AsyncWebServer server(80);

/**
 * ********* FIREBASE *******************************************************************************************************************************************************************************************************************************
 */
FirebaseData fbdoStream;
FirebaseData fbdoControl;
FirebaseData createNode;
FirebaseData removeNode;

FirebaseAuth auth;
FirebaseConfig config;

/**
 * ********* WIFI MODE - STATION ********************************************************************************************************************************************************************************************************************
 */

String mode_ap_ssid = "esp32-";
String mode_ap_pass = "44448888";

void setup()
{
  initResetDeflaut();
  initSignal();

  GEN_ID_BY_MAC.replace(":", "");
  ID_DEVICE = String("device-" + GEN_ID_BY_MAC);

#ifdef DEBUG
  Serial.begin(115200); // init serial port for mode DEBUG
#endif
#ifdef RELEASE
  Serial.begin(115200); // init serial port for mode RELEASE
#endif

#ifdef COLOR
  strip.begin();
  strip.setBrightness(100);
  strip.show();
#endif

#ifdef DEBUG
  Serial.println("");
  Serial.println("[---PROGRAM START---]");
#endif

#ifdef DEBUG
  Serial.println("[DEBUG] - Init Watchdog timer");
#endif
  initWatchdogTimer();

  configTime(0, 0, ntpServer);
  eeprom.begin();

#ifdef DEBUG
  viewEEPROM();
#endif

#ifdef DEBUG
  Serial.println("[DEBUG] - Clear color led");
#endif
#ifdef COLOR
  clearColor();
#endif

#ifdef DEBUG
  Serial.printf("[DEBUG] - Init Number Led Strip RGB - %d\n", eeprom.readNumsLed());
#endif
#ifdef COLOR
  if (eeprom.readNumsLed() > 0 && eeprom.readNumsLed() < 1000)
  {
    strip.updateLength((uint16_t)eeprom.readNumsLed());
  }
#endif

  WiFi.hostname(String(CHost + GEN_ID_BY_MAC));
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(ip, gateway, subnet);
  WiFi.softAP(String(mode_ap_ssid + GEN_ID_BY_MAC).c_str(), mode_ap_pass.c_str());

#ifdef DEBUG
  IPAddress IP = WiFi.softAPIP();
  Serial.printf("AP IP address: %s\n", IP.toString());
#endif

  setupWifiModeStation();

  waitConnectWifi();

  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(STATUS_PIN_WIFI, HIGH);
  }
  else
  {
    digitalWrite(STATUS_PIN_WIFI, LOW);
  }

  setupWebserverModeAP();

  if (WiFi.status() == WL_CONNECTED)
  {
    config.database_url = eeprom.databaseUrl;
    config.signer.test_mode = true;
    // RTDB Stream reconnect timeout (interval) in ms (1 sec - 1 min) when RTDB Stream closed and want to resume.
    config.timeout.rtdbStreamReconnect = 1 * 1000;
    // WiFi reconnect timeout (interval) in ms (10 sec - 5 min) when WiFi disconnected.
    config.timeout.wifiReconnect = 10 * 1000;
    // Socket begin connection timeout (ESP32) or data transfer timeout (ESP8266) in ms (1 sec - 1 min).
    config.timeout.socketConnection = 30 * 1000;
    // Server response read timeout in ms (1 sec - 1 min).
    config.timeout.serverResponse = 10 * 1000;
    // RTDB Stream keep-alive timeout in ms (20 sec - 2 min) when no server's keep-alive event data received.
    config.timeout.rtdbKeepAlive = 45 * 1000;
    // RTDB Stream error notification timeout (interval) in ms (3 sec - 30 sec). It determines how often the readStream
    // will return false (error) when it called repeatedly in loop.
    config.timeout.rtdbStreamError = 3 * 1000;
#ifdef ESP8266
    fbdoStream.setBSSLBufferSize(1024 /* Rx in bytes, 512 - 16384 */, 1024 /* Tx in bytes, 512 - 16384 */);
#endif

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    checkFirebaseInit();
  }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

void loop()
{

  if (globalRestart)
  {
    ESP.restart();
  }

  if (millis() - blinkStart > STATUS_START_DELAY_FLICKER_SHORT) // this is animation status run mode
  {
    if (countBlink < 4)
    {
      blinkStart = millis();
      if (countBlink % 2 == 0)
      {
        digitalWrite(STATUS_PIN_START, HIGH);
      }
      else
      {
        digitalWrite(STATUS_PIN_START, LOW);
      }
      countBlink++;
    }
    else if (millis() - blinkStart > STATUS_START_DELAY_FLICKER_LONG)
    {
      blinkStart = millis();
      countBlink = 0;
    }
  }

  if (!disableBtnReset)
  {

    if (digitalRead(PIN_RESET_DEFLAUT) == LOW && !blockBtnReset)
    {
      timeStartResetPinDetach = millis();
      blockBtnReset = true;
#ifdef DEBUG
      Serial.println("[DEBUG] Start btn HIGH");
#endif
    }

    if (digitalRead(PIN_RESET_DEFLAUT) == HIGH && blockBtnReset)
    {
#ifdef DEBUG
      Serial.println("[DEBUG] Start btn LOW");
#endif
      blockBtnReset = false;
    }

    if (millis() - timeStartResetPinDetach > TIME_RESET_DEFAULT && blockBtnReset)
    {
#ifdef DEBUG
      Serial.println("[DEBUG] Start Reset Default");
#endif

      if (eeprom.DATABASE_NODE.length() > 0)
      {
        Firebase.RTDB.endStream(&fbdoStream);
        clearBufferFirebaseDataAll();

        // ????
        removeNodeEsp();

        // reset link application
        eeprom.saveNodeID("");
        eeprom.saveUserID("");

        // reset wifi configration
        eeprom.savePassword("");
        eeprom.saveSSID("");
      }
      else
      {
#ifdef DEBUG
        Serial.println("[DEBUG] Not found node url to remove!");
#endif
      }

#ifdef LOGIC
      globalRestart = true;
#endif

#ifdef COLOR
      eeprom.saveModeColor(SINGLE);
      requestRestart = true;
      clearEffect = true;
#endif
      disableBtnReset = true;
      blockBtnReset = false;
    }
  }

#ifdef LOGIC
#ifdef RELEASE
  if (Serial.available() > 0)
  {
    uartPicControl = (int)Serial.parseInt();
    if (uartPicControl > 0 && blockControl == false)
    {
      control = uartPicControl % 2 == 0 ? false : true;
      if (uartPicControl < 3)
      {
        idControl = 1;
      }
      else if (uartPicControl < 5)
      {
        idControl = 2;
      }
      else if (uartPicControl < 7)
      {
        idControl = 3;
      }
      if (millis() - timeDebounceLocalControl > 1000)
      {
        timeDebounceLocalControl = millis();
        if (WiFi.status() == WL_CONNECTED)
        {
          controllDevice(idControl, control, true, true);
        }
        else
        {
          controllDevice(idControl, control, false, true);
        }
      }
    }
  }
#endif
  if ((millis() - sendDataPrevMillis > 2500 || sendDataPrevMillis == 0))
  { 
    sendDataPrevMillis = millis();
    if (timerStack[0][1] != NULL && timeControll)
    {
#ifdef DEBUG
      PrintListTimer();
#endif
      epochTime = Get_Epoch_Time();
#ifdef DEBUG
      Serial.println("TimeStamp = " + String(epochTime));
#endif
      bool expire = timerStack[0][1] > epochTime && timerStack[0][1] != NULL ? false : true;
      if (expire)
      {
        unsigned long rangeTimer = epochTime - timerStack[0][1];
        if (rangeTimer < 4)
        {
#ifdef RELEASE
          bool state = timerStack[0][2] == 1 ? true : timerStack[0][2] == 2           ? false
                                                  : stateDevice[timerStack[0][0] - 1] ? false
                                                                                      : true;
          controllDevice(timerStack[0][0], state, true);
#endif
        }
#ifdef DEBUG
        Serial.printf("Controll timer = %d\n", timerStack[0][1]);
        Serial.printf("[Epoch Time] = %d", rangeTimer);
#endif
        removeTimer(timerStack, indexTimerStack, indexTimerStack[0]);
      }
    }
  }
#endif

#ifdef COLOR
  // execute something...
  if (clearEffect)
  {
    clearEffectColor();
    clearEffect = false;
  }
  if (eeprom.modeRGBs == SINGLE && !clearEffect)
  {
    if (acceptChange)
    {
      while (true)
      {
        bool resultR = fadeColor(COLOR_R);
        bool resultG = fadeColor(COLOR_G);
        bool resultB = fadeColor(COLOR_B);
        bool resultALPHA = fadeColor(COLOR_ALPHA);
        setColorChainRGBA(strip.Color(colorPresentState[0], colorPresentState[1], colorPresentState[2]), colorPresentState[3]);
        if (!resultR && !resultG && !resultB && !resultALPHA)
        {
          break;
        }
      }
      acceptChange = false;
    }
  }
  else if (eeprom.modeRGBs == AUTO && !clearEffect && strip.numPixels() != 0)
  {
    strip.setPixelColor(autoIndexI, Wheel(((autoIndexI * 256 / strip.numPixels()) + autoIndexJ) & 255));
    autoIndexI++;
    if (autoIndexI >= strip.numPixels() && millis() - rgbModeAuto > 20)
    {
      rgbModeAuto = millis();
      autoIndexI = 0;
      autoIndexJ++;
      strip.show();
    }
    if (autoIndexJ >= (256 * 5) - 1)
    {
      autoIndexJ = 0;
    }
  }

#endif

#ifdef DEBUG
  if (millis() - timeIntervalCheckRamSize > 2500)
  {
    timeIntervalCheckRamSize = millis();
    checkRam();
  }
#endif

  esp_task_wdt_reset();
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 *   [********* START DEFINE FUNCTION REQUEST *********]
 *
 */

void controllDevices(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!json.isNull())
  {
    JsonObject body = json.as<JsonObject>();

#ifdef LOGIC
    String userId = body["uid"];
    if (eeprom.readUserID() == userId)
    {
      String deviceId = body["did"];
      if ((GEN_ID_BY_MAC + "-1").equals(deviceId))
      {
        bool state = body["state"];
        controllDevice(1, state);
#ifdef DEBUG
        Serial.println("[DEBUG] Controll Switch 1");
#endif
      }
      else if ((GEN_ID_BY_MAC + "-2").equals(deviceId))
      {
        bool state = body["state"];
        controllDevice(2, state);
#ifdef DEBUG
        Serial.println("[DEBUG] Controll Switch 2");
#endif
      }
      else if ((GEN_ID_BY_MAC + "-3").equals(deviceId))
      {
        bool state = body["state"];
        controllDevice(3, state);
#ifdef DEBUG
        Serial.println("[DEBUG] Controll Switch 3");
#endif
      }
      else
      {
        request->send(403, "application/json", "{\"message\":\"ID DEVICE NOT EXIST\"}");
      }
    }
    else
    {
      request->send(403, "application/json", "{\"message\":\"YOU NO PERMISSION\"}");
    }
#endif

#ifdef COLOR
    String userId = body["uid"];
    String deviceId = body["did"];
    if (eeprom.readUserID() == userId && GEN_ID_BY_MAC == deviceId)
    {
      uint8_t mode = body["mode"];
      if (mode != eeprom.modeRGBs)
      {
        eeprom.modeRGBs = mode;
        eeprom.saveModeColor(eeprom.modeRGBs);
#ifdef DEBUG
        Serial.println("[DEBUG] Change Mode Color");
#endif
      }
      if (eeprom.modeRGBs == SINGLE)
      {
        uint8_t colorR = body["value"]["r"];
        uint8_t colorG = body["value"]["g"];
        uint8_t colorB = body["value"]["b"];
        uint8_t colorA = body["value"]["a"];
        colorFultureState[0] = colorR;
        colorFultureState[1] = colorG;
        colorFultureState[2] = colorB;
        colorFultureState[3] = colorA;
        acceptChange = true;
#ifdef DEBUG
        Serial.println("[DEBUG] Change color style value of mde single");
#endif
      }
      else if (eeprom.modeRGBs == AUTO)
      {
        clearEffect = true;
#ifdef DEBUG
        Serial.println("[DEBUG] Change color mode auto");
#endif
      }
    }
    else
    {
      request->send(403, "application/json", "{\"message\":\"YOU NO PERMISSION\"}");
    }
#endif

    request->send(200, "application/json", "{\"message\":\"CONTROLL SUCCESSFULLY\"}");
  }
  else
  {
    request->send(401, "application/json", "{\"message\":\"NOT FOUND PAYLOAD\"}");
  }
}

void pingResponse(AsyncWebServerRequest *request)
{
  String response_str;
  DynamicJsonDocument response_json(400);
  JsonObject payload = response_json.to<JsonObject>();

#ifdef LOGIC
  payload["devices"][0]["id"] = GEN_ID_BY_MAC + "-1";
  payload["devices"][0]["state"] = stateDevice[0];
  payload["devices"][0]["type"] = TYPE_DEVICE;

  payload["devices"][1]["id"] = GEN_ID_BY_MAC + "-2";
  payload["devices"][1]["state"] = stateDevice[1];
  payload["devices"][1]["type"] = TYPE_DEVICE;

  payload["devices"][2]["id"] = GEN_ID_BY_MAC + "-3";
  payload["devices"][2]["state"] = stateDevice[2];
  payload["devices"][2]["type"] = TYPE_DEVICE;

#endif

#ifdef COLOR
  payload["devices"][0]["id"] = GEN_ID_BY_MAC;
  payload["devices"][0]["type"] = TYPE_DEVICE;
  payload["devices"][0]["mode"] = eeprom.modeRGBs;

  payload["devices"][0]["value"]["r"] = colorFultureState[0];
  payload["devices"][0]["value"]["g"] = colorFultureState[1];
  payload["devices"][0]["value"]["b"] = colorFultureState[2];
  payload["devices"][0]["value"]["contrast"] = colorFultureState[3];
#endif

  payload["uid"] = eeprom.readUserID();
  payload["nodeId"] = eeprom.readNodeID();

  serializeJson(payload, response_str);
  request->send(200, "application/json", response_str);
  response_json.clear();
  payload.clear();
}

void restartEsp(AsyncWebServerRequest *request)
{
  request->send(200, "application/json", "{\"message\":\"ESP RESTARTED\"}");
  globalRestart = true;
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// [POST]
void linkAppication(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!json.isNull())
  {
    // if (eeprom.canAccess)
    // {
#ifdef DEBUG
    Serial.println("DB Node = " + eeprom.DATABASE_NODE);
    Serial.println("Wifi Status = " + String(WiFi.status()));
#endif
    JsonObject body = json.as<JsonObject>();
    String idUser = body["idUser"];
    String idNode = body["idNode"];
#ifdef DEBUG
    Serial.println("Link App: idUser = " + idUser + " - idNode = " + idNode);
#endif
    if (idUser.length() > 0 && idNode.length() > 0)
    {
      clearBufferFirebaseDataAll();
      bool stateSaveNodeId = false;
      bool stateSaveUserId = false;
      if (removeNodeEsp())
      {
        stateSaveNodeId = eeprom.saveNodeID(idNode);
        stateSaveUserId = eeprom.saveUserID(idUser);
      }
      esp_task_wdt_reset();
      if (stateSaveNodeId && stateSaveUserId)
      {
        clearBufferFirebaseDataAll();
#ifdef DEBUG
        Serial.println("--- Start Init New Firebase Stream ---");
#endif
        checkFirebaseInit();
        createNode.clear();
        // if (fbdoStream.isStream())
        // {
        request->send(200, "application/json", "{\"message\":\"LINK APP HAS BEEN SUCCESSFULLY\"}");
        // }
        // else
        // {
        //   request->send(400, "application/json", "{\"message\":\"LINK APP HAS BEEN FAILURE\"}");
        // }
      }
      else
      {
        request->send(500, "application/json", "{\"message\":\"FAILURE SAVE PAYLOADS\"}");
      }
    }
    else
    {
      if (idUser.length() <= 0)
      {
        request->send(400, "application/json", "{\"message\":\"ID USER IS NULL\"}");
      }
      else if (idNode.length() <= 0)
      {
        request->send(400, "application/json", "{\"message\":\"ID NODE IS NULL\"}");
      }
      else
      {
        request->send(400, "application/json", "{\"message\":\"PAYLOAD INVALID\"}");
      }
    }
    // }
    // else
    // {
    //   request->send(500, "application/json", "{\"message\":\"SOMETHING WENT WRONG\"}");
    // }
  }
  else
  {
    request->send(403, "application/json", "{\"message\":\"NOT FOUND PAYLOAD\"}");
  }
}

// [POST]
void restartWifiAccess(AsyncWebServerRequest *request)
{
  request->send(200, "application/json", "{\"message\":\"RESTART WIFI ACCESS SUCESSFULLY!\"}");
  countConnectionFailure = 0;
  disconnected = false;
  setupWifiModeStation();
  restartConfigWifi = true;
}

// [POST]
void addConfiguration(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!json.isNull())
  {
    JsonObject body = json.as<JsonObject>();

    String ssid = body["ssid"];
    String password = body["password"];

    String ssidOld = eeprom.readSSID();
    String passwordOld = eeprom.readPassword();

    ssid.trim();
    password.trim();

    uint8_t ssidLen = ssid.length();
    uint8_t passwordLen = password.length();

    bool change = false;

#ifdef DEBUG
    Serial.printf("[DEBUG] SSID: %s, PASSWORD: %s\n", ssid.c_str(), password.c_str());
#endif

    if (ssidLen > 0 && passwordLen > 0)
    {

      if (ssidLen > 32)
      {
        request->send(400, "application/json", "{\"message\":\"SSID WIFI TOO LONG\"}");
        return;
      }
      else if (passwordLen > 64)
      {
        request->send(400, "application/json", "{\"message\":\"PASSWORD WIFI TOO LONG\"}");
        return;
      }

      request->send(200, "application/json", "{\"message\":\"CONFIGURATION WIFI SUCCESSFULLY\"}");

      if (ssid != ssidOld)
      {
        eeprom.saveSSID(ssid);
        change = true;
      }

      if(password != passwordOld) {
        eeprom.savePassword(password);
        change = true;
      }

      if(change) {
        ESP.restart();
      }

    }
    else
    {
      request->send(400, "application/json", "{\"message\":\"PAYLOAD WIFI CONFIG INVALID\"}");
    }
  }
  else
  {
    request->send(400, "application/json", "{\"message\":\"NOT FOUND PAYLOAD\"}");
  }
}

// [POST]
void resetConfigurationWifi(AsyncWebServerRequest *request)
{
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
#ifdef DEBUG
  Serial.println("reset config: wifi = " + ssid + " - password = " + password);
#endif
  if (ssid.length() > 0 && password.length() > 0)
  {
    eeprom.saveSSID("");
    eeprom.savePassword("");
  }
  request->send(200, "application/json", "{\"message\":\"RESET CONFIG SUCCESSFULLY\"}");
}

void resetConfigurationFirebase(AsyncWebServerRequest *request)
{
  String response_str;
  DynamicJsonDocument response_json(200);
  size_t index = 0;

  if (eeprom.saveNodeID(""))
  {
    response_json["list-reset"][index] = "nodeId";
    index++;
  }
  if (eeprom.saveUserID(""))
  {
    response_json["list-reset"][index] = "userId";
    index++;
  }
  eeprom.saveDatabaseUrl("");
  request->send(200, "application/json", "{\"message\":\"RESET CONFIG SUCCESSFULLY\"}");
}

// [GET]
void checkLinkAppication(AsyncWebServerRequest *request)
{
  String nodeID = eeprom.readNodeID();
  String userID = eeprom.readUserID();
#ifdef DEBUG
  Serial.println("check link app: nodeID = " + nodeID + " - userID = " + userID);
#endif
  if (nodeID.length() && userID.length())
  {
    String response_str;
    DynamicJsonDocument response_json(200);
    JsonObject payload = response_json.to<JsonObject>();
    payload["nodeID"] = nodeID;
    payload["userID"] = userID;
    payload["message"] = "CONFIG IS FOUND";
    serializeJson(payload, response_str);
    request->send(200, "application/json", response_str);
    response_json.clear();
  }
  else
  {
    request->send(403, "application/json", "{\"message\":\"CONFIG NOT FOUND\"}");
  }
}

// [GET]
void checkConfiguration(AsyncWebServerRequest *request)
{
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
#ifdef DEBUG
  Serial.println("check config: ssid = " + ssid + " - password = " + password);
#endif
  if (ssid.length() > 0 && password.length() > 0)
  {
    String response_str;
    DynamicJsonDocument response_json(200);
    JsonObject payload = response_json.to<JsonObject>();
    payload["ssid"] = ssid;
    payload["password"] = password;
    payload["ip-station"] = WiFi.localIP();
    payload["status-station"] = WiFi.status();
    payload["quality-station"] = WiFi.RSSI();
    payload["message"] = "WIFI HAS BEEN CONFIG";
    serializeJson(payload, response_str);
    request->send(200, "application/json", response_str);
    response_json.clear();
  }
  else
  {
    request->send(200, "application/json", "{\"message\":\"WIFI NOT YET CONFIG\"}");
  }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 *   [********* {START} DEFINE FUNCTION DEBUG *********]
 *
 */

#ifdef DEBUG

void checkRam()
{
  Serial.printf("Ram size = %d (bytes)\n", ESP.getFreeHeap());
}

void viewEEPROM()
{
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
  String url = eeprom.readDatabaseUrl();
  String nodeID = eeprom.readNodeID();
  String userID = eeprom.readUserID();
  String dbNode = eeprom.DATABASE_NODE;
  String urlNode = eeprom.readDatabaseUrlNode();
#ifdef COLOR
  uint8_t styleColor = eeprom.readModeColor();
  // int numLed = eeprom.readNumsLed();
#endif

  Serial.println("[DEBUG] WIFI NAME = " + ssid + " | length = " + String(ssid.length()));
  Serial.println("[DEBUG] WIFI PASS = " + password + " | length = " + String(password.length()));
  Serial.println("[DEBUG] WIFI DATATBASE URL = " + url + " | length = " + String(url.length()));
  Serial.println("[DEBUG] STATE URL = " + String(eeprom.canAccess));
  Serial.println("[DEBUG] WIFI DATATBASE URL - NODE = " + dbNode + " | length = " + String(dbNode.length()));
  Serial.println("[DEBUG] WIFI NODE ID = " + nodeID + " | length = " + String(nodeID.length()));
#ifdef COLOR
  Serial.println("[DEBUG] MODE COLOR = " + String(styleColor));
  // Serial.println("[DEBUG] NUMs LED = " + String(numLed));
#endif
}

#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 *   [********* {START} DEFINE FUNCTION NODE -> TYPE "LOGIC" *********]
 *
 */

#ifdef LOGIC

#ifdef DEBUG

void PrintListTimer()
{
  Serial.println("\nTimer Unix Stack: ");
  for (size_t i = 0; i < NUMS_TIMER; i++)
  {
    if (timerStack[i][0] == NULL)
    {
      Serial.print("NULL");
      break;
    }
    if (i == 0)
    {
      Serial.print("[");
    }
    for (size_t j = 0; j < 3; j++)
    {
      /* code */
      if (j == 0)
      {
        Serial.print("[");
      }
      Serial.print(String(timerStack[i][j]));
      if (j == 2)
      {
        Serial.print("]");
      }
      else
      {
        Serial.print(", ");
      }
    }
    if (timerStack[i + 1][0] == NULL && timerStack[i + 1][1] == NULL && timerStack[i + 1][2] == NULL)
    {
      Serial.print("]");
      break;
    }
  }
  Serial.println("\nTimer Name Stack: ");
  for (size_t i = 0; i < NUMS_TIMER; i++)
  {
    if (strcmp(indexTimerStack[i], "") == 0)
    {
      Serial.print("NULL");
      break;
    }
    if (i == 0)
    {
      Serial.print("[");
    }
    Serial.print(String(indexTimerStack[i]) + (i == NUMS_TIMER - 1 ? "" : ", "));
    if (strcmp(indexTimerStack[i + 1], "") == 0)
    {
      Serial.print("]");
      break;
    }
  }
  Serial.println();
}

#endif

void removeTimer(unsigned long stack[][3], char stackName[][MAX_NAME_INDEX_FIREBASE], String key, bool isCallBack)
{
  size_t indexFind;
  bool isFind = false;
  unsigned long numDevice;
  for (size_t i = 0; i < NUMS_TIMER; i++)
  {
    if (isFind)
    {
      break;
    }
    if (strcmp(stackName[i], key.c_str()) == 0)
    {
      indexFind = i;
      isFind = true;
      numDevice = stack[i][0];
      for (size_t j = i; j < NUMS_TIMER; j++)
      {
        if (strcmp(stackName[j + 1], "") == 0)
        {
          strcpy(stackName[j], "");
          break;
        }
        strcpy(stackName[j], stackName[j + 1]);
      }
    }
  }
  if (isFind)
  {
    for (size_t i = indexFind; i < NUMS_TIMER; i++)
    {
      if (stack[i + 1][0] == NULL)
      {
        stack[i][0] = NULL;
        stack[i][1] = NULL;
        stack[i][2] = NULL;
        break;
      }
      else
      {
        stack[i][0] = stack[i + 1][0];
        stack[i][1] = stack[i + 1][1];
        stack[i][2] = stack[i + 1][2];
      }
    }
    if (!isCallBack)
    {
      String pathRemove = eeprom.DATABASE_NODE + "/devices/" + ID_DEVICE + "-" + String(numDevice) + "/timer/" + key;
      FirebaseData deleteNode;
      fbdoControl.clear();
      if (ramHeapSize() > SSL_HANDSHAKE_REQUIRE * 1000)
      {
        if (Firebase.RTDB.deleteNode(&deleteNode, pathRemove))
        {
          if (timerStack[0][1] == NULL)
          {
            timeControll = false;
          }
        }
      }
#ifdef DEBUG
      Serial.println("[DELETED PATH] = " + pathRemove);
      Serial.printf("[TIME END] = %d", millis());
#endif
    }
  }
}

void sortTimer(unsigned long stack[][3], char stackName[][MAX_NAME_INDEX_FIREBASE])
{
  char tempSortIndex[MAX_NAME_INDEX_FIREBASE] = "";
  unsigned long tempSort;
  for (size_t i = 0; i < NUMS_TIMER; i++)
  {
    if (stack[i][1] == NULL)
    {
      break;
    }
    for (size_t j = i + 1; j < NUMS_TIMER; j++)
    {
      if (stack[j][1] == NULL)
      {
        break;
      }
      if (stack[j][1] < stack[i][1])
      {
        // assign unix
        tempSort = stack[i][1];
        stack[i][1] = stack[j][1];
        stack[j][1] = tempSort;
        // assign index device
        tempSort = stack[i][0];
        stack[i][0] = stack[j][0];
        stack[j][0] = tempSort;
        // assign control device
        tempSort = stack[i][2];
        stack[i][2] = stack[j][2];
        stack[j][2] = tempSort;

        // assign index name firebase
        strcpy(tempSortIndex, stackName[i]);
        strcpy(stackName[i], stackName[j]);
        strcpy(stackName[j], tempSortIndex);
      }
    }
  }
}

void parserDeviceStatus(FirebaseStream &data, uint8_t numberDevice)
{
  String deviceField = "device-" + GEN_ID_BY_MAC + "-" + numberDevice;
  if (data.jsonObject().get(deviceJson, deviceField + "/state"))
  {
    if (deviceJson.typeNum == 7) // check is boolean
    {
      bool state = deviceJson.to<bool>();
      controllDevice(numberDevice, state);
      stateDevice[numberDevice - 1] = state;
      delay(400);
    }
  }
}

void controllDevice(uint8_t numDevice, bool state, bool syncFirebase, bool ignore)
{
#ifdef RELEASE
  blockControl = true;
  if (!ignore)
  {
    Serial.printf("188%c%d\n", state ? 'n' : 'f', numDevice);
  }
  if (syncFirebase)
  {
    String pathControll = eeprom.DATABASE_NODE + "/devices/" + ID_DEVICE + "-" + String(numDevice) + "/state";
    while (!Firebase.RTDB.setBool(&fbdoControl, pathControll, state))
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        break;
      }
    };
  }
  stateDevice[numDevice - 1] = state;
  blockControl = false;
#endif
}

void parserTimerJson(FirebaseStream &data, uint8_t numberDevice, bool isInit)
{
  String deviceField = "device-" + GEN_ID_BY_MAC + "-" + numberDevice;
  if (isInit)
  {
    if (data.jsonObject().get(timerJson, deviceField + "/timer"))
    {
      timerJson.get<FirebaseJson>(timerParseArray);
      readTimer(timerParseArray, numberDevice);
      timerJson.clear();
    }
  }
  else
  {
    readTimer(data.jsonObject(), numberDevice, data.dataPath().substring(29, 49));
  }
}

void readTimer(FirebaseJson &fbJson, uint8_t numberDevice, String keyAdd)
{
  size_t numTimerPayload = fbJson.iteratorBegin();
  FirebaseJson::IteratorValue timerItem;
  size_t indexStart;
  for (size_t i = 0; i < NUMS_TIMER; i++)
  {
    if (timerStack[i][0] == NULL && timerStack[i][1] == NULL && timerStack[i][2] == NULL)
    {
      indexStart = i;
      break;
    }
  }
  for (size_t i = 0; i < numTimerPayload; i++)
  {
    if (indexStart < 30)
    {
      timerItem = fbJson.valueAt(i);
      if (timerItem.key.equals("unix"))
      {
        timerStack[indexStart][0] = numberDevice;
        timerStack[indexStart][1] = timerItem.value.toInt();
      }
      else if (timerItem.key.equals("value"))
      {
        timerStack[indexStart][2] = timerItem.value.toInt();
        if (keyAdd.length() > 0)
        {
          strcpy(indexTimerStack[indexStart], keyAdd.c_str());
        }
        indexStart++;
      }
      else
      {
        strcpy(indexTimerStack[indexStart], timerItem.key.c_str());
      }
    }
    else
    {
      break;
    }
  }
  fbJson.iteratorEnd();
}

#endif

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 *   [********* {START} DEFINE FUNCTION NODE -> TYPE "COLOR" *********]
 *
 */
#ifdef COLOR
// ..... define here ...

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

bool fadeColor(INDEX_COLOR index)
{
  if (colorPresentState[index] > colorFultureState[index])
  {
    colorPresentState[index] = --colorPresentState[index];
    return true;
  }
  else if (colorPresentState[index] < colorFultureState[index])
  {
    colorPresentState[index] = ++colorPresentState[index];
    return true;
  }
  else
  {
    return false;
  }
}

void setColorChainRGBA(uint32_t c, uint8_t a)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.setBrightness(a);
  strip.show();
}

void clearColor()
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}

void clearEffectColor(uint8_t brightness)
{
  strip.setBrightness(brightness);
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    esp_task_wdt_reset();
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}

void parserColorRGB(FirebaseStream &data, String prefix)
{
  data.jsonObject().get(deviceJson, prefix + "r");
  uint8_t colorR = deviceJson.to<int>();
  colorFultureState[0] = colorR;
  deviceJson.clear();
  data.jsonObject().get(deviceJson, prefix + "g");
  uint8_t colorG = deviceJson.to<int>();
  colorFultureState[1] = colorG;
  deviceJson.clear();
  data.jsonObject().get(deviceJson, prefix + "b");
  uint8_t colorB = deviceJson.to<int>();
  colorFultureState[2] = colorB;
  deviceJson.clear();
  data.jsonObject().get(deviceJson, prefix + "contrast");
  uint8_t alpha = deviceJson.to<int>();
  colorFultureState[3] = alpha;
  deviceJson.clear();
  acceptChange = true;
}

uint8_t parserColorMode(FirebaseStream &data, bool init)
{
  if (init)
  {
    data.jsonObject().get(deviceJson, "mode");
    return deviceJson.to<uint8_t>();
  }
  else
  {
    return data.to<uint8_t>();
  }
}

int parserNumsLed(FirebaseStream &data, bool init)
{
  if (init)
  {
    data.jsonObject().get(deviceJson, "nums-Led");
    return deviceJson.intValue;
  }
  else
  {
    return data.intData();
  }
}

#endif
/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */

/**
 *   [********* {START} DEFINE FUNCTION GENERAL *********]
 *
 */

void initSignal()
{
  pinMode(STATUS_PIN_START, OUTPUT);
  pinMode(STATUS_PIN_WIFI, OUTPUT);

  digitalWrite(STATUS_PIN_START, LOW);
  digitalWrite(STATUS_PIN_WIFI, LOW);
}

void initResetDeflaut()
{
  pinMode(PIN_RESET_DEFLAUT, INPUT_PULLUP);
}

uint32_t ramHeapSize()
{
  return ESP.getFreeHeap();
}

void clearBufferFirebaseDataAll()
{
  fbdoStream.clear();
  fbdoControl.clear();
  createNode.clear();
  removeNode.clear();
}

void setupStreamFirebase()
{
  esp_task_wdt_reset();
  Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);
  String pathStream;
#ifdef LOGIC
  pathStream = eeprom.DATABASE_NODE + "/devices";
#endif
#ifdef COLOR
  pathStream = eeprom.DATABASE_NODE + "/devices/device-" + GEN_ID_BY_MAC;
#ifdef DEBUG
  Serial.println("[DEBUG] Path Stream = " + pathStream);
#endif
#endif
  if (pathStream.length() > 0)
  {
    if (!Firebase.RTDB.beginStream(&fbdoStream, pathStream))
    {
#ifdef DEBUG
      Serial_Printf("stream begin error, %s\n\n", fbdoStream.errorReason().c_str());
#endif
    }
    else
    {
#ifdef DEBUG
      Serial.println(String("Stream OK!"));
#endif
    }
  }
}

void streamCallback(FirebaseStream data)
{
#ifdef DEBUG
  Serial_Printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  Serial.println();
  Serial_Printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
#endif
  String dataPath = data.dataPath();
  uint8_t dataType = data.dataTypeEnum();
#ifdef LOGIC
  uint8_t numDevice = (uint8_t)(dataPath.substring(21, 22).toInt());
  if (dataType == d_boolean)
  {
    if (dataPath.indexOf("state") > 0 || numDevice) // execute controll turn on device by [INDEX]
    {
      if (data.dataTypeEnum() == fb_esp_rtdb_data_type_boolean)
      {
        bool state = data.to<boolean>();
        if (!blockControl)
        {
          controllDevice(numDevice, state);
        }
      }
    }
  }
  else if (dataType == d_json)
  {
    if (dataPath.equals("/") && isFirst)
    { // execute init state all device
#ifdef DEBUG
      Serial.println("Init Status");
#endif
      parserTimerJson(data, 1);
      parserTimerJson(data, 2);
      parserTimerJson(data, 3);

      sortTimer(timerStack, indexTimerStack);

      parserDeviceStatus(data, 1);
      parserDeviceStatus(data, 2);
      parserDeviceStatus(data, 3);
      timeControll = true;
      isFirst = false;
    }
    else if (dataPath.indexOf("timer") > 0 && numDevice)
    {
      parserTimerJson(data, numDevice, false); // add timer to list timer
      sortTimer(timerStack, indexTimerStack);
      timeControll = true;
    }
#ifdef DEBUG
    PrintListTimer();
#endif
  }
  else if (dataType == d_null)
  {
    removeTimer(timerStack, indexTimerStack, dataPath.substring(29, 49), true);
#ifdef DEBUG
    PrintListTimer();
#endif
  }
#endif
#ifdef COLOR

  if (dataType == d_json)
  {
    if (dataPath == "/")
    {

#ifdef DEBUG
      Serial.println("[DEBUG] Data Mode Color = " + String(eeprom.modeRGBs));
#endif
      int numsLed = parserNumsLed(data, true);
      uint8_t modeColor = parserColorMode(data, true);

#ifdef DEBUG
      Serial.println("[DEBUG] Data Num Led Change = " + String(numsLed));
#endif
      if (eeprom.readModeColor() != modeColor)
      {
        eeprom.saveModeColor(modeColor);
      }

      if (numsLed != 0)
      {
        // eeprom.saveNumsLed(numsLed);
        strip.updateLength((uint16_t)numsLed);
        clearEffect = true;
      }

      if (eeprom.modeRGBs == SINGLE)
      {
        // Parser Data led RGB here

#ifdef DEBUG
        Serial.println("[DEBUG] Data Color = " + data.jsonString());
#endif
        parserColorRGB(data, "value/");
      }
    }
    else if (dataPath.indexOf("value") > 0)
    {

#ifdef DEBUG
      Serial.println("[DEBUG] Data Color = " + data.jsonString());
#endif
      parserColorRGB(data);
    }
  }
  else if (dataType == d_integer)
  {
    if (dataPath.indexOf("mode") > 0)
    {
      eeprom.modeRGBs = parserColorMode(data);
      eeprom.saveModeColor(eeprom.modeRGBs);

      if (eeprom.modeRGBs == SINGLE)
      {
        colorPresentState[0] = 0;
        colorPresentState[1] = 0;
        colorPresentState[2] = 0;
        colorPresentState[3] = 0;
        acceptChange = true;
      }

      // acceptChange = true;
      // clearEffect = true;

#ifdef DEBUG
      Serial.println("[DEBUG] Data Mode Color = " + String(eeprom.modeRGBs));
#endif
    }
    else if (dataPath == "/nums-Led")
    {
      int numsLed = parserNumsLed(data);

#ifdef DEBUG
      Serial.println("[DEBUG] Data Num Led Change = " + String(numsLed));
#endif
      // eeprom.saveNumsLed(numsLed);

      if (numsLed < numLedPresent)
      {
        clearEffectColor();
      }

      strip.updateLength((uint16_t)numsLed);

      if(numLedPresent != numsLed) {
        if (eeprom.modeRGBs == SINGLE)
        {
          colorPresentState[0] = 0;
          colorPresentState[1] = 0;
          colorPresentState[2] = 0;
          colorPresentState[3] = 0;
          acceptChange = true;
        }else {
          clearEffect = true;
        }
      }
      numLedPresent = numsLed;
    }
  }

#endif
}

void streamTimeoutCallback(bool timeout)
{
#ifdef DEBUG
  if (timeout)
  {
    Serial.println("Stream timed out, resuming...\n");
  }
  else
  {
  }
  if (!fbdoStream.httpConnected())
  {
    Serial_Printf("error code: %d, reason: %s\n\n", fbdoStream.httpCode(), fbdoStream.errorReason().c_str());
  }
#endif
}

void checkFirebaseInit()
{
  // [Check] - NodeID is exist

  if (eeprom.canAccess)
  {
    String pathDevice = String(eeprom.DATABASE_NODE + "/devices");
    bool statePath = Firebase.RTDB.pathExisted(&fbdoStream, pathDevice);
#ifdef DEBUG
    Serial.println(String("PATH Device = " + String(pathDevice)));
    Serial.println(String("State Path = " + String(statePath)));
#endif
    if (!statePath)
    {
      jsonNewDevice.clear();
#ifdef LOGIC
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-1/state", false);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-1/type", TYPE_DEVICE);

      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-2/state", false);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-2/type", TYPE_DEVICE);

      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-3/state", false);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-3/type", TYPE_DEVICE);
#endif

#ifdef COLOR
      // ... init device COLOR
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "/value/r", colorFultureState[0]);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "/value/g", colorFultureState[1]);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "/value/b", colorFultureState[2]);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "/value/contrast", colorFultureState[3]);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "/type", TYPE_DEVICE);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "/mode", (uint8_t)SINGLE);
#endif

      esp_task_wdt_reset();
      if (Firebase.RTDB.setJSON(&createNode, pathDevice, &jsonNewDevice))
      {
        createNode.clear();
      }

#ifdef DEBUG
      Serial.println("[CREATED] - NEW LIST DEVICE");
#endif
    }
    if (fbdoStream.isStream())
    {
      Firebase.RTDB.endStream(&fbdoStream);
    }
    setupStreamFirebase();
  }
}

void setupWifiModeStation()
{
  // ACTIVE MODE AP
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
#ifdef DEBUG
  Serial.println("<--- STATION WIFI --->");
  Serial.println("[SSID] = " + String(ssid));
  Serial.println("[PASSWORD] = " + String(password));
#endif
  if (ssid.length() > 0 && password.length() > 0)
  {
    if (WiFi.isConnected())
    {
      WiFi.disconnect();
    }
    WiFi.begin(ssid.c_str(), password.c_str());
  }
}

unsigned int strLength(const char *str)
{
  return sizeof(str) / sizeof(char);
}

void setupWebserverModeAP()
{
  // [GET] - ROUTE: '/' => Render UI interface
  // server.serveStatic("/", LittleFS, "/index.minify.html");
  // [GET] - ROUTE: '/reset-config' => Reset config WIFI
  // server.on("/scan-network", HTTP_GET, scanListNetwork);
  // [GET] - ROUTE: '/ping' => Pong to request
  server.on("/ping", HTTP_GET, pingResponse);
  // [GET] - ROUTE: '/is-config' => Check WIFI is configuration
  server.on("/is-config", HTTP_GET, checkConfiguration);
  // [GET] - ROUTE: '/restart' => restart ESP
  server.on("/restart", HTTP_GET, restartEsp);
  // [POST] - ROUTE: '/reset-config-wifi' => Reset config WIFI
  server.on("/reset-config-wifi", HTTP_POST, resetConfigurationWifi);
  // [POST] - ROUTE: '/reset-config-firebase' => Reset config firebase
  server.on("/reset-config-firebase", HTTP_POST, resetConfigurationFirebase);
  // [POST] - ROUTE: '/reset-config-firebase' => restart wifi access
  server.on("/restart-wifi-access", HTTP_POST, restartWifiAccess);
  // [POST] - ROUTE: '/controll' => Controll Device
  AsyncCallbackJsonWebHandler *handlerControllDevices = new AsyncCallbackJsonWebHandler("/controll", controllDevices);
  // [POST] - ROUTE: '/config-wifi' => Goto config WIFI save below EEPROM
  AsyncCallbackJsonWebHandler *handlerAddConfig = new AsyncCallbackJsonWebHandler("/config-wifi", addConfiguration);
  // [POST] - ROUTE: '/link-app' => Goto config firebase save below EEPROM
  AsyncCallbackJsonWebHandler *handlerLinkApp = new AsyncCallbackJsonWebHandler("/link-app", linkAppication);
  // [GET] - ROUTE: '/is-link-app' => Check configuration link-app
  server.on("/is-link-app", HTTP_GET, checkLinkAppication);
  // [GET] - ROUTE: '/*any' => Check configuration link-app
  server.onNotFound(notFound);

  // START WEBSERVER
  server.addHandler(handlerControllDevices);
  server.addHandler(handlerAddConfig);
  server.addHandler(handlerLinkApp);
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();
}

unsigned long Get_Epoch_Time()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

bool removeNodeEsp()
{
  bool result = false;
  if (ramHeapSize() > SSL_HANDSHAKE_REQUIRE * 1000)
  {
#ifdef DEBUG
    Serial.println("[DEBUG] Remove Database node: " + eeprom.DATABASE_NODE);
#endif
    if (Firebase.RTDB.deleteNode(&removeNode, eeprom.DATABASE_NODE))
    {
      result = true;
#ifdef DEBUG
      Serial.println("[DEBUG] Remove Database node successfull!");
#endif
    }
    else
    {
#ifdef DEBUG
      Serial.println("[DEBUG] Remove Database node failure!");
#endif
    }
  }
  else
  {

#ifdef DEBUG
    Serial.println("[DEBUG] Heap ram not enough to handshake ssl delete node!");
#endif
  }
  return result;
}

void initWatchdogTimer()
{
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch
}

status_config_wifi connectWifiByConfig(String ssid, String password)
{
  if (ssid.length() > 0 && password.length() > 0)
  { // connection has been password
#ifdef DEBUG
    Serial.println("[DEBUG] connection has been \"password\"");
#endif
    if (ssid.length() > 32)
    {
      return SSID_TOO_LONG;
    }
    if (password.length() > 64)
    {
      return PASSWORD_TOO_LONG;
    }

    if (WiFi.isConnected())
    {
      WiFi.disconnect();

#ifdef DEBUG
      Serial.printf("[DEBUG] Diable wifi connecting\n");
#endif
    }

    WiFi.begin(ssid.c_str(), password.c_str());

    return CONFIG_DONE;
  }
  else if (ssid.length() > 0)
  { // connection just have ssid
#ifdef DEBUG
    Serial.println("[DEBUG] connection just have \"ssid\"");
#endif
    if (ssid.length() > 32)
    {
      return SSID_TOO_LONG;
    }

    if (WiFi.isConnected())
    {
      WiFi.disconnect();

#ifdef DEBUG
      Serial.printf("[DEBUG] Diable wifi connecting\n");
#endif
    }

    WiFi.begin(ssid.c_str());

    return CONFIG_DONE;
  }
  else
  {
    return CONFIG_NOT_FOUND;
  }
}

wl_status_t waitConnectWifi()
{
  uint8_t countWaitConnection = 0;
  unsigned long wifiStatus = 0;
  while (WiFi.status() != WL_CONNECTED && countWaitConnection < WAIT_CONNECTION)
  {
    if (millis() - wifiStatus > 500)
    {
      wifiStatus = millis();
      esp_task_wdt_reset();
      countWaitConnection++;
#ifdef DEBUG
      Serial.println("[Pooling check] Status Wifi " + String(WiFi.status()));
#endif
    }
  }
  return WiFi.status();
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ */
