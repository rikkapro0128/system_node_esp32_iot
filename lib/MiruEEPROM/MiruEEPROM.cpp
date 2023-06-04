
#include "MiruEEPROM.h"

EepromMiru::EepromMiru(int size, String url_database)
{
  this->size = size;
  this->databaseUrl = url_database;
}

void EepromMiru::begin()
{
  EEPROM.begin(this->size);
  this->DATABASE_NODE = this->readDatabaseUrlNode();
  this->checkAccess();
  this->modeRGBs = this->readModeColor();
  this->numsLed = this->readNumsLed();
}

void EepromMiru::resetAll()
{
  for (unsigned int i = 0; i < this->size; i++)
  {
    EEPROM.write(i, NULL);
    delay(5);
  }
  EEPROM.commit();
  this->updateDatabaseNode("", "");
}

String EepromMiru::readRaw()
{
  String temp;
  for (unsigned int i = 0; i < this->size; i++)
  {
    uint8_t num = EEPROM.read(i);
    temp = temp + String((char)num);
  }
  return temp;
}

bool EepromMiru::saveModeColor(uint8_t mode)
{
  this->modeRGBs = mode;
  EEPROM.write(this->addr_ModeColor, mode);
  EEPROM.commit();
  return true;
}
bool EepromMiru::saveNumsLed(int nums)
{
  if (nums > 0 && nums < 1000)
  {
    this->maxCharacter = String(nums).length();
    bool state = this->checkWrite(this->addr_NumsLed, String(nums), this->numsLed);
    this->maxCharacter = 0;
    return state;
  }
  else
  {
    return false;
  }
}
bool EepromMiru::saveSSID(String ssid)
{
  return this->checkWrite(this->addr_ssid, ssid, this->ssid);
}
bool EepromMiru::savePassword(String password)
{
  return this->checkWrite(this->addr_password, password, this->password);
}
bool EepromMiru::saveUserID(String user_id)
{
  bool state = this->checkWrite(this->addr_userID, user_id, this->userID);
  this->updateDatabaseNode(user_id, this->readNodeID());
  return state;
}
bool EepromMiru::saveNodeID(String node_id)
{
  bool state = this->checkWrite(this->addr_NodeID, node_id, this->NodeID);
  this->updateDatabaseNode(this->readUserID(), node_id);
  return state;
}
bool EepromMiru::saveDatabaseUrl(String url)
{
  this->maxCharacter = 120;
  bool state = this->checkWrite(this->addr_databaseUrl, url, this->databaseUrl);
  this->maxCharacter = 0;
  return state;
}

uint8_t EepromMiru::readModeColor()
{
  uint8_t character = EEPROM.read(this->addr_ModeColor);
  return character;
}
int EepromMiru::readNumsLed()
{
  this->maxCharacter = 3;
  String temp = this->checkRead(this->addr_NumsLed, this->numsLed);
  this->maxCharacter = 0;
  return (int)temp.toInt();
}
String EepromMiru::readSSID()
{
  return this->checkRead(this->addr_ssid, this->ssid);
}
String EepromMiru::readPassword()
{
  return this->checkRead(this->addr_password, this->password);
}
String EepromMiru::readNodeID()
{
  return this->checkRead(this->addr_NodeID, this->NodeID);
}
String EepromMiru::readUserID()
{
  return this->checkRead(this->addr_userID, this->userID);
}
String EepromMiru::readDatabaseUrl()
{
  this->maxCharacter = 120;
  String temp = this->checkRead(this->addr_databaseUrl, this->databaseUrl);
  this->maxCharacter = 0;
  return temp;
}
String EepromMiru::readDatabaseUrlNode()
{
  return String("/user-" + this->readUserID() + "/nodes/node-" + this->readNodeID());
}

String EepromMiru::checkRead(int addr, String &cacheValue)
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

String EepromMiru::readKey(int addr)
{
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
  return temp;
}

bool EepromMiru::writeKey(int addr, String tmp, uint8_t limit)
{
  int len_str = tmp.length();
  uint8_t checkLimit = this->maxCharacter ? this->maxCharacter : limit;
  if (len_str > checkLimit)
  {
    return false;
  }
  else
  {
    int start = 0;
    for (uint8_t i = addr; i < (addr + checkLimit); i++)
    {
      if (i < (addr + len_str))
      {
        EEPROM.write(i, (uint8_t)tmp[start]);
        delay(5);
        start++;
      }
      else
      {
        EEPROM.write(i, NULL);
        delay(5);
      }
    }
    EEPROM.commit();
    return true;
  }
}

bool EepromMiru::checkWrite(int addr, String tmp, String &cached)
{
  bool check = this->writeKey(addr, tmp);
  if (check)
  {
    cached = tmp;
  }
  return check;
}

void EepromMiru::updateDatabaseNode(String uid, String nid)
{
  this->DATABASE_NODE = String("/user-" + uid + "/nodes/node-" + nid);
  this->checkAccess();
}
void EepromMiru::checkAccess()
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
