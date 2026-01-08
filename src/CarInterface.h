#pragma once

#include "LiveData.h"

class CommInterface; // Forward declaration

class CarInterface
{

protected:
  LiveData *liveData;
  CommInterface *commInterface;

public:
  void setLiveData(LiveData *pLiveData);
  void setCommInterface(CommInterface *pCommInterface);
  virtual void activateCommandQueue();
  virtual bool commandAllowed() { return true; }
  virtual void parseRowMerged();
  virtual void loadTestData();
  //
  virtual void testHandler(const String &cmd);
  //
  virtual std::vector<String> customMenu(int16_t menuId);
  virtual void carCommand(const String &cmd);
};
