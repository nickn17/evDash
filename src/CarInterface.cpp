#include "CarInterface.h"
#include "LiveData.h"

void CarInterface::setLiveData(LiveData *pLiveData)
{
  liveData = pLiveData;
}

void CarInterface::setCommInterface(CommInterface *pCommInterface)
{
  commInterface = pCommInterface;
}

void CarInterface::activateCommandQueue()
{
}

void CarInterface::parseRowMerged()
{
}

void CarInterface::loadTestData()
{
}

void CarInterface::testHandler(const String &cmd)
{
}

std::vector<String> CarInterface::customMenu(int16_t menuId)
{
  return {};
}

void CarInterface::carCommand(const String &cmd)
{
}
