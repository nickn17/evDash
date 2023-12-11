#pragma once

#include "LiveData.h"

class BoardInterface; // Forward declaration

class WebInterface
{
protected:
public:
  void init(LiveData *pLiveData, BoardInterface *pBoard);
  virtual void mainLoop();
  //static void handleRoot();
  //static void handleNotFound();
};
