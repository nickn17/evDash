#pragma once

#include <FS.h>
#include "LiveData.h"
#include "CarInterface.h"
#include "CommInterface.h"

class BoardInterface {

  protected:
    LiveData* liveData;
    CarInterface* carInterface;
    CommInterface* commInterface;
  public:
    // Screens, buttons
    byte displayScreenCount = 7;
    bool btnLeftPressed   = true;
    bool btnMiddlePressed = true;
    bool btnRightPressed  = true;
    bool testDataMode = false;
    bool scanDevices = false;
    String sdcardRecordBuffer = "";
    //
    void setLiveData(LiveData* pLiveData);
    void attachCar(CarInterface* pCarInterface);
    virtual void initBoard()=0;
    virtual void wakeupBoard()=0;
    virtual void afterSetup()=0;
    virtual void mainLoop()=0;
    virtual bool isButtonPressed(int button) {return false;};
    virtual void enterSleepMode(int secs)=0;
    virtual bool skipAdapterScan() {return false;};
    bool carCommandAllowed() { return carInterface->commandAllowed(); }
    void showTime();
    virtual void setTime(String timestamp);
    virtual void ntpSync()=0;
    // Graphics & GUI
    virtual void displayMessage(const char* row1, const char* row2)=0;
    virtual void turnOffScreen()=0;
    virtual void setBrightness()=0;
    virtual void redrawScreen()=0;
    void parseRowMerged();
    // Menu
    virtual void showMenu()=0;
    virtual void hideMenu()=0;
    // Common
    void shutdownDevice();
    virtual void otaUpdate()=0;
    void saveSettings();
    void resetSettings();
    void loadSettings();
    void customConsoleCommand(String cmd);
    // Sdcard
    virtual bool sdcardMount() {return false; }; 
    virtual void sdcardToggleRecording()=0;
    bool serializeParamsToJson(File file, bool inclApiKey = false);
};
