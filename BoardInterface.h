#ifndef BOARDINTERFACE_H
#define BOARDINTERFACE_H

#include "LiveData.h"
#include "CarInterface.h"

class BoardInterface {

  private:
  public:
    // Screens, buttons
    byte displayScreen = SCREEN_AUTO;
    byte displayScreenAutoMode = 0;
    byte displayScreenSpeedHud = false;
    byte displayScreenCount = 7;
    bool btnLeftPressed   = true;
    bool btnMiddlePressed = true;
    bool btnRightPressed  = true;
    bool testDataMode = false;
    bool scanDevices = false;
    // Debug screen - next command with right button
    uint16_t debugCommandIndex = 0;
    String debugAtshRequest = "ATSH7E4";
    String debugCommandRequest = "220101";
    String debugLastString = "620101FFF7E7FF99000000000300B10EFE120F11100F12000018C438C30B00008400003864000035850000153A00001374000647010D017F0BDA0BDA03E8";
    String debugPreviousString = "620101FFF7E7FFB3000000000300120F9B111011101011000014CC38CB3B00009100003A510000367C000015FB000013D3000690250D018E0000000003E8";
    //
    LiveData* liveData;
    CarInterface* carInterface;
    void setLiveData(LiveData* pLiveData);
    void attachCar(CarInterface* pCarInterface);
    virtual void initBoard()=0;
    virtual void afterSetup()=0;
    virtual void mainLoop()=0;
    virtual bool skipAdapterScan() {return false;};
    // Graphics & GUI
    virtual void displayMessage(const char* row1, const char* row2)=0;
    virtual void setBrightness(byte lcdBrightnessPerc)=0;
    virtual void redrawScreen()=0;
    // Menu
    virtual void showMenu()=0;
    virtual void hideMenu()=0;
    // Common
    void shutdownDevice();
    void saveSettings();
    void resetSettings();
    void loadSettings();
//    void customConsoleCommand(String cmd);
};

#endif // BOARDINTERFACE_H
