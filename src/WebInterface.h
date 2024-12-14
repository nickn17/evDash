#pragma once

#include "LiveData.h"

class BoardInterface; // Forward declaration

class WebInterface
{
public:
  void init(LiveData *pLiveData, BoardInterface *pBoard);
  virtual void mainLoop();
};
