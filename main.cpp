#ifdef SWITCH
#include <switch.h>
#endif

#include "libs/chesto/src/Container.hpp"
#include "libs/chesto/src/TextElement.hpp"

class WifiInfo {
    std::string ssid;
    std::string auth;
};

bool readWifiInfo() {
#ifdef SWITCH
    nifmInitialize(NifmServiceType_System);
    NifmNetworkProfileData data;
    Result res = nifmGetCurrentNetworkProfile(&data)
    return true;
#endif
    return false;
}

int main(int argc, char* argv[])
{
	RootDisplay* display = new RootDisplay();
    display->backgroundColor = fromRGB(0x00, 0x88, 0xA3);
    Container* con = new Container(COL_LAYOUT, 5);

    bool success = readWifiInfo();

    if (success) {
        con->add(new TextElement("ssid: hello", 30));
        con->add(new TextElement("auth: ********", 30));
    } else {
        con->add(new TextElement("Issue reading wifi info, make sure a main connection is established!", 30));
    }

    con->centerIn(display);
    display->child(con);

    display->canUseSelectToExit = true;
	return display->mainLoop();
}
