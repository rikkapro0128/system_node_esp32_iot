#pragma once

#include <EEPROM.h>

class EepromMiru
{
public:
  String DATABASE_NODE = "";
  String databaseUrl = "";
  bool canAccess = false;
  uint8_t modeRGBs = 0;
  String numsLed = "";
  EepromMiru(int size, String url_database = "");
  void begin();
  void resetAll();
  String readRaw();
  bool saveModeColor(uint8_t mode);
  bool saveNumsLed(int nums);
  bool saveSSID(String ssid);
  bool savePassword(String password);
  bool saveUserID(String user_id);
  bool saveNodeID(String node_id);
  bool saveDatabaseUrl(String url);

  uint8_t readModeColor();
  int readNumsLed();

  String readSSID();
  String readPassword();
  String readNodeID();
  String readUserID();
  String readDatabaseUrl();
  String readDatabaseUrlNode();

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
  int addr_ModeColor = 320;
  int addr_NumsLed = 325;

  String checkRead(int addr, String &cacheValue);
  String readKey(int addr);
  bool writeKey(int addr, String tmp, uint8_t limit = 50);
  bool checkWrite(int addr, String tmp, String &cached);
  void updateDatabaseNode(String uid = "", String nid = "");
  void checkAccess();
};
