#include <sstream>
#include <iostream>

#if defined(SWITCH)
#include <switch.h>
#elif defined(__WIIU__)
#include <nsysnet/netconfig.h>
#endif

#include "./libs/libqrencode/qrenc.c"

#include "libs/chesto/src/Container.hpp"
#include "libs/chesto/src/TextElement.hpp"
#include "libs/chesto/src/Button.hpp"
#include "libs/chesto/src/Constraint.hpp"
#include "libs/chesto/src/ImageElement.hpp"

#include "headers.h"

int monoOffset = 4;

class WifiInfo {
public:
    std::string ssid = "N/A";
    std::string auth = "N/A";
    bool isWPA = false;
    bool isWEP = false;
    bool isHidden = false;
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
            wifiInfo->isWPA = true;
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
            wifiInfo->isWPA = true;
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
#elif defined(WII) || defined(WII_MOCK)
    netconfig_t config;

#if WII_MOCK
    // use normal open and read
    FILE* file = fopen("./config.dat", "rb");
    if (file == NULL) {
        printf("Error opening file\n");
        return false;
    }
    fread(&config, sizeof(netconfig_t), 1, file);
    fclose(file);
#else
    // init libogc
    WII_Initialize();

    // open the wii config path
    s32 file = IOS_Open(WII_CONFIG_PATH, IPC_OPEN_READ);
    if (file < 0) {
        fprintf(stderr, "Error opening file: %d\n", file);
        return false;
    }

    // read config info from file
    s32 bytesRead = IOS_Read(file, &config, sizeof(netconfig_t));
    if (bytesRead < 0) {
        fprintf(stderr, "Error reading file: %d\n", bytesRead);
        IOS_Close(file);
        return false;
    }
    IOS_Close(file);
#endif

    // check if we're online
    if (config.header4 == 0) {
        printf("Not connected to the internet\n");
        return false;
    }

    int connectionIndex = 0;
    // search the connections for the one that is connected and wireless
    for (int i = 0; i < 3; i++) {
        if ((config.connection[i].flags >> 7) & 0x01 && config.connection[i].flags & 0x01 == 0) {
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
    return true;
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
        row2->add(new Button("Show", X_BUTTON, true))->setAction([row1, row2, wifiInfo, font_size, con]() {
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

    con->height += 60;

    auto escapeSpecialChars = [](std::string str) {
        // escape ;, :, comma, and \ in the string
        std::ostringstream ss;
        for (char c : str) {
            if (c == ';' || c == ':' || c == ',' || c == '\\') {
                ss << '\\';
            }
            ss << c;
        }
        return ss.str();
    };

    auto buildWifiString = [escapeSpecialChars](WifiInfo* wifiInfo) {
        // escape ;, :, and , in the ssid
        std::ostringstream ss;
        ss << "WIFI:";
        if (wifiInfo->isWPA) {
            ss << "T:WPA;";
        } else if (wifiInfo->isWEP) {
            ss << "T:WEP;";
        } else {
            ss << "T:nopass;";
        }
        ss << "S:" << escapeSpecialChars(wifiInfo->ssid) << ";";
        if (wifiInfo->auth.length() > 0) {
            ss << "P:" << escapeSpecialChars(wifiInfo->auth) << ";";
        }
        ss << "H:false;;"; // TODO: handle this?
        return ss.str();
    };

    // create a QR code image element
    // write the QR code png
    std::string imgKey = "qr_showifi_tmp.png";
    std::string qrString = buildWifiString(&wifiInfo);

    // std::cout << "QR String: " << qrString << std::endl;

    margin = 1;
    QRcode* qrCode = encode(reinterpret_cast<const unsigned char*>(qrString.c_str()), qrString.length());
    writePNG(qrCode, imgKey.c_str(), PNG32_TYPE);

    ImageElement* qrImage = new ImageElement(imgKey);
    qrImage->setSize(200, 200);
    con->add(qrImage);

    // delete the image file
    std::remove(imgKey.c_str());
    QRcode_free(qrCode);

    con->height += 10;

    TextElement* qrText = new TextElement("Scan to share network setup!", font_size - font_mod);
    con->add(qrText);

    // center these after the container is fully built
    qrText->centerHorizontallyIn(con);
    qrImage->centerHorizontallyIn(con);

    con->centerIn(display);
    display->child(con);

    // put the quit button in the bottom right
    auto quitBtn = new Button("Close App", B_BUTTON, true);
    quitBtn->setAction([](){
		RootDisplay::mainDisplay->requestQuit();
	});
    quitBtn->constrain(ALIGN_BOTTOM | ALIGN_RIGHT, 35);
    display->child(quitBtn);


    // if we're on 3ds, we have to offset the container to target only the bottom screen
    #if defined(_3DS) || defined(_3DS_MOCK)
    con->y += 240 / 2 - 10; // this works because the two screens are both 240 pixels tall
    #endif

    display->canUseSelectToExit = true;
	return display->mainLoop();
}
