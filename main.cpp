#include <sstream>
#include <iostream>

#if defined(SWITCH)
#include <switch.h>
#elif defined(__WIIU__)
#include <nsysnet/netconfig.h>
#endif

#include "libs/chesto/src/Container.hpp"
#include "libs/chesto/src/TextElement.hpp"
#include "libs/chesto/src/Button.hpp"
#include "libs/chesto/src/Constraint.hpp"

#include "headers.h"

int monoOffset = 4;

class WifiInfo {
public:
    std::string ssid = "N/A";
    std::string auth = "N/A";
    bool unsupported = false;
};

bool readWifiInfo(WifiInfo* wifiInfo) {
#if defined(SWITCH)
    nifmInitialize(NifmServiceType_System);
    NifmNetworkProfileData profileData;
    Result res = nifmGetCurrentNetworkProfile(&profileData);
    nifmExit();

    monoOffset = 10; // for some reason, the monospaced font is further offset on switch

    if (R_SUCCEEDED(res)) {
        auto data = profileData.wireless_setting_data;
        char* ssid = data.ssid;
        ssid[data.ssid_len] = '\0';
        if (data.ssid_len > 0) {
            wifiInfo->ssid = std::string(ssid);
        }
        char* pass = (char*)(data.passphrase) + 1; // TODO: unicode support?
        // TODO: passphrase length may not be part of this field in the future
        int pass_len = int(data.passphrase[0]);
        pass[pass_len] = '\0';
        if (pass_len > 0) {
            wifiInfo->auth = std::string(pass);
        }
        return true;
    }
#elif defined(__WIIU__)
    NetConfWifiConfig wifiConfig;
    netconf_init();
    int success = netconf_get_wifi_cfg(&wifiConfig);
    netconf_close();

    if (success >= 0) {
        auto config = wifiConfig.config;
        auto ssid_len = config.ssidlength;
        char* ssid = (char*)config.ssid;
        ssid[ssid_len] = '\0';
        if (ssid_len > 0) {
            wifiInfo->ssid = std::string(ssid);
        }
        auto privacy = config.privacy;
        auto pass_len = privacy.aes_key_len;
        char* pass = (char*)privacy.aes_key;
        pass[pass_len] = '\0';
        if (pass_len > 0) {
            wifiInfo->auth = std::string(pass);
        }
        return true;
    }
#elif defined(_3DS)
    acInit();
    u32 wifiStatus;
    ACU_GetWifiStatus(&wifiStatus);
    if (wifiStatus > 0) { // 1 = o3ds, 2 = n3ds, 0 = off
        u32 ssidLen = 0;
        ACU_GetSSIDLength(&ssidLen);
        char* ssid = (char*)malloc(ssidLen + 1);
        ACU_GetSSID(ssid);
        ssid[ssidLen] = '\0';
        if (ssidLen > 0) {
            wifiInfo->ssid = std::string(ssid);
        }
        // TODO: look up password, using parsing methods in headers.h
        return true;
    }
#elif defined(WII)
    // init libogc
    WII_Initialize();

    // open the wii config path
    FILE* file = fopen(WII_CONFIG_PATH, "rb");
    // read the entire file contents into the netconfig_t struct
    netconfig_t config;
    fread(&config, sizeof(netconfig_t), 1, file);
    fclose(file);

    // check if we're online
    if (config.header4 == 0) {
        return false;
    }

    int connectionIndex = 0;
    // search the connections for the one that is connected and wireless
    for (int i = 0; i < 3; i++) {
        if ((config.connection[i].flags >> 31) & 0x01 && config.connection[i].flags & 0x01 == 0) {
            connectionIndex = i;
            break;
        }
    }

    connection_t connection = config.connection[connectionIndex];

    // get the ssid
    int ssidLen = connection.ssid_length;
    if (ssidLen > 0) {
        u8* ssid = connection.ssid;
        wifiInfo->ssid = std::string(reinterpret_cast<char*>(ssid), ssidLen);
    }

    // get the password (TODO: check encryption type?)
    int keyLen = connection.key_length;
    if (keyLen > 0) {
        u8* key = connection.key;
        wifiInfo->auth = std::string(reinterpret_cast<char*>(key), keyLen);
    }

    return true;
#else
    // we aren't a supported platform, so we don't have wifi info
    wifiInfo->unsupported = true;
#endif
    return false;
}

int main(int argc, char* argv[])
{
	RootDisplay* display = new RootDisplay();
    Container* con = new Container(COL_LAYOUT, 8);

    WifiInfo wifiInfo;
    bool success = readWifiInfo(&wifiInfo);

    // can be used to test on PC
    // success = true;

    int font_size = 30;
    int font_mod = 6; // for +1 and -1 headings/subtext
    #if defined(_3DS) || defined(_3DS_MOCK)
    // 3DS has a smaller screen
    font_size = 15;
    font_mod = 2;
    #endif

    if (success)
    {        
        Container* row1 = new Container(ROW_LAYOUT, 6);
        row1->add(new TextElement("SSID:", font_size));
        row1->add(new TextElement(wifiInfo.ssid.c_str(), font_size, 0, MONOSPACED))->y += monoOffset;

        Container* row2 = new Container(ROW_LAYOUT, 6);
        row2->add(new TextElement("Auth:", font_size));
        row2->add(new Button("Show", X_BUTTON))->setAction([row1, row2, wifiInfo, font_size, con]() {
            // replace button with the password
            auto btn = row2->elements.back();
            auto x = btn->x;
            auto y = btn->y + monoOffset;
            row2->elements.pop_back();
            row2->elements.push_back(
                (new TextElement(wifiInfo.auth.c_str(), font_size, 0, MONOSPACED, SCREEN_WIDTH - btn->xAbs - 40))
                    ->setPosition(x, y)
            );
        });

        con->add(row1);
        con->add(row2);
    } else
    {
        if (wifiInfo.unsupported) {
            con->add(new TextElement("Unsupported platform!", font_size + font_mod))->constrain(ALIGN_CENTER_HORIZONTAL);
            con->add(new TextElement("This build will never return any WiFi info.", font_size))->constrain(ALIGN_CENTER_HORIZONTAL);
        } else {
            con->add(new TextElement("Issue reading WiFi info!", font_size + font_mod))->constrain(ALIGN_CENTER_HORIZONTAL);
            con->add(new TextElement("This app only works on the current network.", font_size - font_mod))->constrain(ALIGN_CENTER_HORIZONTAL);
            con->add(new TextElement("Please connect to a hotspot and retry.", font_size - font_mod))->constrain(ALIGN_CENTER_HORIZONTAL);
        }
        con->width = SCREEN_WIDTH;
    }

    con->height += 10;

    con->add(new Button("Close App", B_BUTTON))->centerHorizontallyIn(con)->setAction([display](){
		display->exitRequested = true;
		display->isRunning = false;
	});

    con->centerIn(display);
    display->child(con);

    // if we're on 3ds, we have to offset the container to target only the bottom screen
    #if defined(_3DS) || defined(_3DS_MOCK)
    con->y += 240 / 2 - 10; // this works because the two screens are both 240 pixels tall
    #endif

    display->canUseSelectToExit = true;
	return display->mainLoop();
}
