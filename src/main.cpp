#include <EEPROM.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <addons/RTDBHelper.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#define TIMER_INTERVAL_MS 1000

// GLOBAL VARIABLE
#define SSL_HANDSHAKE_REQUIRE 100
#define ERROR_TIMER 10
#define PIN_OUT 3
#define NUMS_TIMER 30
#define MAX_NAME_INDEX_FIREBASE 25
#define POOLING_WIFI 5000

#define _DEBUG_
#define _RELEASE_

const char *ntpServer = "pool.ntp.org";
const char *CHost = "plant.io";
String getMac = WiFi.macAddress();
String GEN_ID_BY_MAC = String(getMac);
String ID_DEVICE;
String TYPE_DEVICE = "LOGIC";
String removeAfter;
bool STATUS_PIN = false;
String DATABASE_URL = "";
uint32_t ramSize;
float_t flashSize = 80000;
float_t percent = 100;
bool reConnect = false;
bool reLinkApp = false;
bool resetConfig = false;
bool timeControll = false;
bool isFirst = true;
bool blockControl = false;
bool control = true;
bool handTouch = false;
bool missingCallBack = false;
bool startRemovePath = false;
size_t timeDebounceLocalControl = 0;
size_t numTimer = 0;
size_t timeoutWifi = 0;
size_t idControl = 0;
// int signal = 0;
int idTimerRuning;
unsigned long sendDataPrevMillis = 0;
unsigned long timerStack[NUMS_TIMER][3];
bool stateDevice[3] = {false, false, false};
bool stateDeviceFirebase[3] = {false, false, false};
char indexTimerStack[NUMS_TIMER][MAX_NAME_INDEX_FIREBASE];
unsigned long epochTime;
FirebaseJson jsonNewDevice;
FirebaseJson timerParseArray;
FirebaseJsonData timerJson;
FirebaseJsonData deviceJson;

// JSON DOCUMENT
DynamicJsonDocument bufferBodyPaserModeAP(400);

// FUNCTION PROTOTYPE - COMPONENT

// => DEBUG FUNC

#ifdef _DEBUG_
void checkRam();
void viewEEPROM();
void PrintListTimer();
#endif

// => RELEASE FUNC

void setupStreamFirebase();
void checkFirebaseInit();
void setupWifiModeStation();
void setupWebserverModeAP();
void checkLinkAppication(AsyncWebServerRequest *request);
void linkAppication(AsyncWebServerRequest *request, JsonVariant &json);
void addConfiguration(AsyncWebServerRequest *request, JsonVariant &json);
void resetConfigurationWifi(AsyncWebServerRequest *request);
void resetConfigurationFirebase(AsyncWebServerRequest *request);
void checkConfiguration(AsyncWebServerRequest *request);
void notFound(AsyncWebServerRequest *request);
float_t ramHeapSize();
void clearBufferFirebaseDataAll();
unsigned long Get_Epoch_Time();
void parserTimerJson(FirebaseStream &data, uint8_t numberDevice, bool isInit = true);
void parserDeviceStatus(FirebaseStream &data, uint8_t numberDevice);
void streamCallback(FirebaseStream data);
void streamTimeoutCallback(bool timeout);
void controllDevice(uint8_t numDevice, bool state, bool syncFirebase = false, bool ignore = false);
void readTimer(FirebaseJson &fbJson, uint8_t numberDevice, String keyAdd = "");
void removeTimer(unsigned long stack[][3], char stackName[][MAX_NAME_INDEX_FIREBASE], String key, bool isCallBack = false);
void sortTimer(unsigned long stack[][3], char stackName[][MAX_NAME_INDEX_FIREBASE]);
unsigned int strLength(const char *str);

class EepromMiru
{

public:
  String DATABASE_NODE = "";
  String databaseUrl = "";
  bool canAccess = false;

  EepromMiru(int size, String url_database = "")
  {
    this->size = size;
    this->databaseUrl = url_database;
  }

  void begin()
  {
    this->DATABASE_NODE = this->readDatabaseUrlNode();
    this->checkAccess();
  }

  void resetAll()
  {
    EEPROM.begin(this->size);
    for (unsigned int i = 0; i < this->size; i++)
    {
      EEPROM.write(i, NULL);
    }
    EEPROM.end();
    this->updateDatabaseNode("", "");
  }

  String readRaw()
  {
    EEPROM.begin(this->size);
    String temp;
    for (unsigned int i = 0; i < this->size; i++)
    {
      uint8_t num = EEPROM.read(i);
      temp = temp + String((char)num);
    }
    EEPROM.end();
    return temp;
  }

  bool saveSSID(String ssid)
  {
    return this->checkWrite(this->addr_ssid, ssid, this->ssid);
  }
  bool savePassword(String password)
  {
    return this->checkWrite(this->addr_password, password, this->password);
  }
  bool saveUserID(String user_id)
  {
    bool state = this->checkWrite(this->addr_userID, user_id, this->userID);
    this->updateDatabaseNode(user_id, this->readNodeID());
    return state;
  }
  bool saveNodeID(String node_id)
  {
    bool state = this->checkWrite(this->addr_NodeID, node_id, this->NodeID);
    this->updateDatabaseNode(this->readUserID(), node_id);
    return state;
  }
  bool saveDatabaseUrl(String url)
  {
    this->maxCharacter = 120;
    bool state = this->checkWrite(this->addr_databaseUrl, url, this->databaseUrl);
    this->maxCharacter = 0;
    return state;
  }

  String readSSID()
  {
    return this->checkRead(this->addr_ssid, this->ssid);
  }
  String readPassword()
  {
    return this->checkRead(this->addr_password, this->password);
  }
  String readNodeID()
  {
    return this->checkRead(this->addr_NodeID, this->NodeID);
  }
  String readUserID()
  {
    return this->checkRead(this->addr_userID, this->userID);
  }
  String readDatabaseUrl()
  {
    this->maxCharacter = 120;
    String temp = this->checkRead(this->addr_databaseUrl, this->databaseUrl);
    this->maxCharacter = 0;
    return temp;
  }
  String readDatabaseUrlNode()
  {
    return String("/user-" + this->readUserID() + "/nodes/node-" + this->readNodeID());
  }

private:
  unsigned int size = 1024;
  String ssid = "";
  int addr_ssid = 0;
  String password = "";
  int addr_password = 50;
  String userID = "";
  int addr_userID = 100;
  String NodeID = "";
  int addr_NodeID = 150;
  int addr_databaseUrl = 200;
  uint8_t maxCharacter = 0;

  String checkRead(int addr, String &cacheValue)
  {
    if (cacheValue.length() > 0)
    {
      return cacheValue;
    }
    else
    {
      String temp = this->readKey(addr);
      if (temp != cacheValue)
      {
        cacheValue = temp;
      }
      return temp;
    }
  }

  String readKey(int addr)
  {
    EEPROM.begin(this->size);
    String temp = "";
    for (unsigned int i = addr; i < this->maxCharacter || (addr + 50); i++)
    {
      uint8_t character = EEPROM.read(i);
      if (character <= 126 && character >= 32)
      {
        temp = temp + String((char)character);
      }
      else
      {
        break;
      }
    }
    EEPROM.end();
    return temp;
  }

  bool writeKey(int addr, String tmp, uint8_t limit = 50)
  {
    int len_str = tmp.length();
    uint8_t checkLimit = this->maxCharacter ? this->maxCharacter : limit;
#ifdef _DEBUG_
    Serial.println(checkLimit);
#endif
    if (len_str > checkLimit)
    {
      return false;
    }
    else
    {
      EEPROM.begin(this->size);
      int start = 0;
      for (uint8_t i = addr; i < addr + checkLimit; i++)
      {
        if (i < (addr + len_str))
        {
          EEPROM.write(i, (uint8_t)tmp[start]);
          start++;
        }
        else
        {
          EEPROM.write(i, NULL);
        }
      }
      EEPROM.commit();
      EEPROM.end();
      return true;
    }
  }

  bool checkWrite(int addr, String tmp, String &cached)
  {
    bool check = this->writeKey(addr, tmp);
    if (check)
    {
      cached = tmp;
    }
    return check;
  }

  void updateDatabaseNode(String uid = "", String nid = "")
  {
    this->DATABASE_NODE = String("/user-" + uid + "/nodes/node-" + nid);
    this->checkAccess();
  }
  void checkAccess()
  {
    String nodeId = this->readNodeID();
    String userId = this->readUserID();
    String dbURL = this->readDatabaseUrl();
    if (nodeId.length() > 0 && userId.length() > 0 && dbURL.length() > 0)
    {
      this->canAccess = true;
    }
    else
    {
      this->canAccess = false;
    }
  }
};

// [********* INSTANCE *********]

// => EEPROM
EepromMiru eeprom(300, "esp8266-device-db-default-rtdb.firebaseio.com");

// => SERVER
AsyncWebServer server(80);

// => FIREBASE
FirebaseData fbdoStream;
FirebaseData fbdoControl;
FirebaseData createNode;

FirebaseAuth auth;
FirebaseConfig config;

// WIFI MODE - AP
String mode_ap_ssid = "esp32-miru";
String mode_ap_pass = "44448888";
String mode_ap_ip = "192.168.1.120";
String mode_ap_gateway = "192.168.1.1";
String mode_ap_subnet = "255.255.255.0";
String pathFile = "/";

// WIFI MODE - STATION

void setup()
{
  GEN_ID_BY_MAC.replace(":", "");
  ID_DEVICE = String("device-" + GEN_ID_BY_MAC);

#ifdef _DEBUG_
  Serial.begin(115200);
#endif
#ifdef _RELEASE_
  Serial.begin(115200);
#endif

#ifdef _DEBUG_
  Serial.println("");
  Serial.println("[---PROGRAM START---]");
#endif

  configTime(0, 0, ntpServer);
  eeprom.begin();

#ifdef _DEBUG_
  viewEEPROM();
#endif

  WiFi.hostname(CHost);
  WiFi.mode(WIFI_AP_STA);
  WiFi.persistent(true);
  WiFi.softAP(mode_ap_ssid.c_str(), mode_ap_pass.c_str());

#ifdef _DEBUG_
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
#endif

  setupWifiModeStation();

  while (WiFi.status() != WL_CONNECTED)
  {
    if (timeoutWifi > 12)
    {
      WiFi.disconnect(true);
      break;
    }
    timeoutWifi++;
    delay(500);
  }

  setupWebserverModeAP();

  config.database_url = eeprom.databaseUrl;
  config.signer.test_mode = true;
#ifdef ESP8266
  fbdoStream.setBSSLBufferSize(1024 /* Rx in bytes, 512 - 16384 */, 1024 /* Tx in bytes, 512 - 16384 */);
#endif
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  checkFirebaseInit();
}

void loop()
{
  // #ifdef _RELEASE_
  // if (Serial.available() > 0)
  // {
  //   signal = (int)Serial.parseInt();
  //   if (signal > 0 && blockControl == false)
  //   {
  //     control = signal % 2 == 0 ? false : true;
  //     if (signal < 3)
  //     {
  //       idControl = 1;
  //     }
  //     else if (signal < 5)
  //     {
  //       idControl = 2;
  //     }
  //     else if (signal < 7)
  //     {
  //       idControl = 3;
  //     }
  //     if (millis() - timeDebounceLocalControl > 1000)
  //     {
  //       timeDebounceLocalControl = millis();
  //       if (WiFi.status() == WL_CONNECTED)
  //       {
  //         controllDevice(idControl, control, true, true);
  //       }
  //       else
  //       {
  //         controllDevice(idControl, control, false, true);
  //       }
  //     }
  //   }
  // }
  // #endif
  if ((millis() - sendDataPrevMillis > 2500 || sendDataPrevMillis == 0))
  {
#ifdef _DEBUG_
    checkRam();
#endif
    sendDataPrevMillis = millis();
    if (timerStack[0][1] != NULL && timeControll)
    {
#ifdef _DEBUG_
      PrintListTimer();
#endif
      epochTime = Get_Epoch_Time();
      bool expire = timerStack[0][1] > epochTime && timerStack[0][1] != NULL ? false : true;
      if (expire)
      {
        unsigned long rangeTimer = epochTime - timerStack[0][1];
        if (rangeTimer < 4)
        {
#ifdef _RELEASE_
          bool state = timerStack[0][2] == 1 ? true : timerStack[0][2] == 2           ? false
                                                  : stateDevice[timerStack[0][0] - 1] ? false
                                                                                      : true;
          controllDevice(timerStack[0][0], state, true);
#endif
        }
#ifdef _DEBUG_
        Serial.printf("Controll timer = %d\n", timerStack[0][1]);
        Serial.printf("[Epoch Time] = %d", rangeTimer);
#endif
        removeTimer(timerStack, indexTimerStack, indexTimerStack[0]);
        if (timerStack[0][1] == NULL)
        {
          timeControll = false;
        }
      }
    }
  }
  if (startRemovePath)
  {
#ifdef _DEBUG_
    Serial.println("--- Start remove ---");
#endif
    clearBufferFirebaseDataAll();
    if (ramHeapSize() > SSL_HANDSHAKE_REQUIRE)
    {
      if (Firebase.RTDB.deleteNode(&fbdoStream, removeAfter))
      {
#ifdef _DEBUG_
        Serial.println("--- Remove Doned ---");
#endif
        // setupStreamFirebase();
        removeAfter = "";
        startRemovePath = false;
      }
      else
      {
#ifdef _DEBUG_
        Serial.println("--- Can Not Remove ---");
#endif
      }
    }
    else
    {
#ifdef _DEBUG_
      Serial.println("--- Ram Not Enouge to SSL ---");
#endif
    }
  }
  if (reLinkApp && !startRemovePath)
  {
    clearBufferFirebaseDataAll();
#ifdef _DEBUG_
    Serial.println("--- Start Init New Firebase Stream ---");
#endif
    checkFirebaseInit();
    if (!fbdoStream.isStream())
    {
#ifdef _DEBUG_
      Serial.println("--- Restart Init New Firebase Stream ---");
#endif
      setupStreamFirebase();
    }
    reLinkApp = false;
  }
  if (resetConfig)
  {
    clearBufferFirebaseDataAll();
    Firebase.RTDB.deleteNode(&fbdoStream, removeAfter);
    resetConfig = false;
  }
}

#ifdef _DEBUG_
void viewEEPROM()
{
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
  String url = eeprom.readDatabaseUrl();
  String nodeID = eeprom.readNodeID();
  String userID = eeprom.readUserID();
  String dbNode = eeprom.DATABASE_NODE;
  String urlNode = eeprom.readDatabaseUrlNode();

  Serial.println("WIFI NAME = " + ssid + " | length = " + String(ssid.length()));
  Serial.println("WIFI PASS = " + password + " | length = " + String(password.length()));
  Serial.println("WIFI DATATBASE URL = " + url + " | length = " + String(url.length()));
  Serial.println("STATE URL = " + String(eeprom.canAccess));
  Serial.println("WIFI DATATBASE URL - NODE = " + dbNode + " | length = " + String(dbNode.length()));
  Serial.println("WIFI NODE ID = " + nodeID + " | length = " + String(nodeID.length()));
  Serial.println("WIFI USER ID = " + userID + " | length = " + String(userID.length()));
}
#endif

void setupStreamFirebase()
{
  Firebase.RTDB.setStreamCallback(&fbdoStream, streamCallback, streamTimeoutCallback);

  if (!Firebase.RTDB.beginStream(&fbdoStream, eeprom.DATABASE_NODE + "/devices"))
  {
#ifdef _DEBUG_
    Serial_Printf("stream begin error, %s\n\n", fbdoStream.errorReason().c_str());
#endif
  }
  else
  {
#ifdef _DEBUG_
    Serial.println(String("Stream OK!"));
#endif
  }
}

void streamCallback(FirebaseStream data)
{
#ifdef _DEBUG_
  Serial.println(ESP.getFreeHeap());
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
#ifdef _DEBUG_
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
#ifdef _DEBUG_
    PrintListTimer();
#endif
  }
  else if (dataType == d_null)
  {
    removeTimer(timerStack, indexTimerStack, dataPath.substring(29, 49), true);
#ifdef _DEBUG_
    PrintListTimer();
#endif
  }
}

#ifdef _DEBUG_
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
    Serial.print(String(indexTimerStack[i]) + (i == numTimer - 1 ? "" : ", "));
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
      Firebase.RTDB.deleteNode(&deleteNode, pathRemove);
#ifdef _DEBUG_
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
#ifdef _RELEASE_
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

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
#ifdef _DEBUG_
    Serial.println("stream timed out, resuming...\n");
#endif
  }

  if (!fbdoStream.httpConnected())
  {
#ifdef _DEBUG_
    Serial_Printf("error code: %d, reason: %s\n\n", fbdoStream.httpCode(), fbdoStream.errorReason().c_str());
#endif
  }
}

void checkFirebaseInit()
{
  // [Check] - NodeID is exist

  if (eeprom.canAccess)
  {
    String pathDevice = String(eeprom.DATABASE_NODE + "/devices");
    bool statePath = Firebase.RTDB.pathExisted(&fbdoStream, pathDevice);
#ifdef _DEBUG_
    Serial.println(String("PATH Device = " + String(pathDevice)));
    Serial.println(String("State Path = " + String(statePath)));
#endif
    if (!statePath)
    {
      jsonNewDevice.clear();

      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-1/state", false);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-1/type", TYPE_DEVICE);

      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-2/state", false);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-2/type", TYPE_DEVICE);

      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-3/state", false);
      jsonNewDevice.set("device-" + GEN_ID_BY_MAC + "-3/type", TYPE_DEVICE);

      if (Firebase.RTDB.setJSON(&createNode, pathDevice, &jsonNewDevice))
      {
        createNode.clear();
      }

#ifdef _DEBUG_
      Serial.println("[CREATED] - NEW LIST DEVICE");
#endif
    }
    setupStreamFirebase();
  }
}

void setupWifiModeStation()
{
  // ACTIVE MODE AP
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
#ifdef _DEBUG_
  Serial.println("<--- STATION WIFI --->");
  Serial.println("[SSID] = " + String(ssid));
  Serial.println("[PASSWORD] = " + String(password));
#endif
  if (ssid.length() > 0 && password.length() > 0)
  {
    if (WiFi.isConnected())
    {
      WiFi.disconnect(true);
    }
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);
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
  // [GET] - ROUTE: '/is-config' => Check WIFI is configuration
  server.on("/is-config", HTTP_GET, checkConfiguration);
  // [POST] - ROUTE: '/reset-config-wifi' => Reset config WIFI
  server.on("/reset-config-wifi", HTTP_POST, resetConfigurationWifi);
  server.on("/reset-config-firebase", HTTP_POST, resetConfigurationFirebase);
  // [POST] - ROUTE: '/config-wifi' => Goto config WIFI save below EEPROM
  AsyncCallbackJsonWebHandler *handlerAddConfig = new AsyncCallbackJsonWebHandler("/config-wifi", addConfiguration);
  // [POST] - ROUTE: '/link-app' => Goto config firebase save below EEPROM
  AsyncCallbackJsonWebHandler *handlerLinkApp = new AsyncCallbackJsonWebHandler("/link-app", linkAppication);
  // [GET] - ROUTE: '/is-link-app' => Check configuration link-app
  server.on("/is-link-app", HTTP_GET, checkLinkAppication);
  // [GET] - ROUTE: '/*any' => Check configuration link-app
  server.onNotFound(notFound);

  // START WEBSERVER
  server.addHandler(handlerAddConfig);
  server.addHandler(handlerLinkApp);
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

// [********* Func Request *********]

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// [POST]
void linkAppication(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!json.isNull())
  {
    if (eeprom.canAccess)
    {
#ifdef _DEBUG_
      Serial.println("DB Node = " + eeprom.DATABASE_NODE);
      Serial.println("Wifi Status = " + String(WiFi.status()));
#endif
      removeAfter = eeprom.DATABASE_NODE;
      JsonObject body = json.as<JsonObject>();
      String idUser = body["idUser"];
      String idNode = body["idNode"];
#ifdef _DEBUG_
      Serial.println("Link App: idUser = " + idUser + " - idNode = " + idNode);
#endif
      if (idUser.length() > 0 && idNode.length() > 0)
      {
        bool stateSaveNodeId = eeprom.saveNodeID(idNode);
        bool stateSaveUserId = eeprom.saveUserID(idUser);
        if (stateSaveNodeId && stateSaveUserId)
        {
          request->send(200, "application/json", "{\"message\":\"LINK APP HAS BEEN SUCCESSFULLY\"}");
          reLinkApp = true;
          if (removeAfter.length() > 0)
          {
            startRemovePath = true;
          }
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
    }
    else
    {
      request->send(500, "application/json", "{\"message\":\"SOMETHING WENT WRONG\"}");
    }
  }
  else
  {
    request->send(403, "application/json", "{\"message\":\"NOT FOUND PAYLOAD\"}");
  }
}

// [POST]
void addConfiguration(AsyncWebServerRequest *request, JsonVariant &json)
{
  if (!json.isNull())
  {
    JsonObject body = json.as<JsonObject>();

    String ssid = body["ssid"];
    String password = body["password"];
    eeprom.saveSSID(ssid);
    eeprom.savePassword(password);
    // Serial.println("[Save config EEPROM]");
    request->send(200, "application/json", "{\"message\":\"CONFIGURATION WIFI SUCCESSFULLY\"}");
    setupWifiModeStation();
  }
  else
  {
    request->send(403, "application/json", "{\"message\":\"NOT FOUND PAYLOAD\"}");
  }
}

// [POST]
void resetConfigurationWifi(AsyncWebServerRequest *request)
{
  String ssid = eeprom.readSSID();
  String password = eeprom.readPassword();
#ifdef _DEBUG_
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

  removeAfter = eeprom.DATABASE_NODE;
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
  resetConfig = true;
  request->send(200, "application/json", "{\"message\":\"RESET CONFIG SUCCESSFULLY\"}");
}

// [GET]
void checkLinkAppication(AsyncWebServerRequest *request)
{
  String nodeID = eeprom.readNodeID();
  String userID = eeprom.readUserID();
#ifdef _DEBUG_
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
#ifdef _DEBUG_
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

float_t ramHeapSize()
{
  ramSize = ESP.getFreeHeap();
  return ((float_t)ramSize / flashSize * percent);
}
#ifdef _DEBUG_
void checkRam()
{
  ramSize = ESP.getFreeHeap();
#if defined(_DEBUG_) || defined(_RELEASE_)
  Serial.println(String(((float_t)ramSize / flashSize * percent)) + " (kb)");
#endif
}
#endif

void clearBufferFirebaseDataAll()
{
  fbdoStream.clear();
  fbdoControl.clear();
  createNode.clear();
}
