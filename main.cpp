#include <iostream>
#include <sstream>

#if defined(SWITCH)
#include <switch.h>
#elif defined(__WIIU__)
#include <nsysnet/netconfig.h>
#endif

#include "./libs/libqrencode/qrencode.h"

#include "libs/chesto/src/Button.hpp"
#include "libs/chesto/src/Constraint.hpp"
#include "libs/chesto/src/Container.hpp"
#include "libs/chesto/src/ImageElement.hpp"
#include "libs/chesto/src/TextElement.hpp"

#include "headers.h"

int monoOffset = 8;

class WifiInfo {
public:
  std::string ssid = "N/A";
  std::string auth = "N/A";
  bool isWPA = false;
  bool isWEP = false;
  bool isHidden = false;
  bool unsupported = false;
};

bool readWifiInfo(WifiInfo *wifiInfo) {
#if defined(SWITCH)
  nifmInitialize(NifmServiceType_System);
  NifmNetworkProfileData profileData;
  Result res = nifmGetCurrentNetworkProfile(&profileData);
  nifmExit();

  if (R_SUCCEEDED(res)) {
    auto data = profileData.wireless_setting_data;
    char *ssid = data.ssid;
    ssid[data.ssid_len] = '\0';
    if (data.ssid_len > 0) {
      wifiInfo->ssid = std::string(ssid);
    }
    char *pass = (char *)(data.passphrase) + 1; // TODO: unicode support?
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
    char *ssid = (char *)config.ssid;
    ssid[ssid_len] = '\0';
    if (ssid_len > 0) {
      wifiInfo->ssid = std::string(ssid);
    }
    auto privacy = config.privacy;
    auto pass_len = privacy.aes_key_len;
    char *pass = (char *)privacy.aes_key;
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
    char *ssid = (char *)malloc(ssidLen + 1);
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
  FILE *file = fopen("./config.dat", "rb");
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
    if ((config.connection[i].flags >> 7) & 0x01 &&
        config.connection[i].flags & 0x01 == 0) {
      connectionIndex = i;
      break;
    }
  }

  connection_t connection = config.connection[connectionIndex];

  // get the ssid
  int ssidLen = connection.ssid_length;
  if (ssidLen > 0) {
    u8 *ssid = connection.ssid;
    wifiInfo->ssid = std::string(reinterpret_cast<char *>(ssid), ssidLen);
  }

  // get the password (TODO: check encryption type?)
  int keyLen = connection.key_length;
  if (keyLen > 0) {
    u8 *key = connection.key;
    wifiInfo->auth = std::string(reinterpret_cast<char *>(key), keyLen);
  }

  return true;
#else
  // we aren't a supported platform, so we don't have wifi info
  wifiInfo->unsupported = true;
#endif
  return false;
}

int main(int argc, char *argv[]) {
  using namespace Chesto;

  auto display = std::make_unique<RootDisplay>();
  auto con = display->createNode<Container>(COL_LAYOUT, 25);

  WifiInfo wifiInfo;
  bool success = readWifiInfo(&wifiInfo);

  // can be used to test on PC
  //   success = true;

  int font_size = 30;
  int font_mod = 6; // for +1 and -1 headings/subtext
#if defined(_3DS) || defined(_3DS_MOCK)
  // 3DS has a smaller screen
  font_size = 15;
  font_mod = 2;
#endif

  if (success) {
    auto row1 = con->createNode<Container>(ROW_LAYOUT, 12);
    row1->createNode<TextElement>("SSID:", font_size);
    row1->createNode<TextElement>(wifiInfo.ssid.c_str(), font_size, nullptr,
                                  MONOSPACED)
        ->y += monoOffset;

    auto row2 = con->createNode<Container>(ROW_LAYOUT, 12);
    row2->createNode<TextElement>("Auth:", font_size);
    auto btn = row2->createNode<Button>("Show", X_BUTTON, true);
    btn->setAction([btn, row2, wifiInfo, font_size]() {
      // replace button with the password (hide it, add text at same position)
      btn->hidden = true;
      auto textElem = std::make_unique<TextElement>(
          wifiInfo.auth.c_str(), font_size, nullptr, MONOSPACED,
          SCREEN_WIDTH - btn->xAbs - 40);
      textElem->setPosition(btn->x, btn->y + monoOffset);
      row2->addNode(std::move(textElem));
    });

#if !defined(_3DS) && !defined(_3DS_MOCK)
    // add a bit of a spacer to our container first (TODO: why do we need to do
    // this?)
    con->height += 110;
#endif

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

    auto buildWifiString = [escapeSpecialChars](WifiInfo *wifiInfo) {
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
      if (wifiInfo->isWPA || wifiInfo->isWEP) {
        ss << "P:" << escapeSpecialChars(wifiInfo->auth) << ";";
      }
      ss << "H:false;;"; // TODO: handle this? (hidden network)
      return ss.str();
    };

    // create a QR code directly to SDL texture
    std::string qrString = buildWifiString(&wifiInfo);
    std::cout << "QR String: " << qrString << std::endl;

    QRcode *qrCode =
        QRcode_encodeString(qrString.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);

    auto height = qrCode->width + 2;
    auto width = qrCode->width + 2;
    auto newSurface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

    // fill the background with white
    SDL_FillRect(newSurface, NULL,
                 SDL_MapRGB(newSurface->format, 255, 255, 255));

    for (int y = 0; y < qrCode->width; y++) {
      for (int x = 0; x < qrCode->width; x++) {
        // lowest bit = 1 is a black pixel
        if (qrCode->data[y * qrCode->width + x] & 1) {
          SDL_Rect rect = {y + 1, x + 1, 1, 1};
          SDL_FillRect(newSurface, &rect,
                       SDL_MapRGB(newSurface->format, 0, 0, 0));
        }
      }
    }

    // create a Chesto texture from the surface
    CST_SetQualityHint("nearest");
    auto qrTexture = std::make_unique<Texture>();
    qrTexture->loadFromSurface(newSurface);
    SDL_FreeSurface(newSurface);
    QRcode_free(qrCode);

    qrTexture->setSize(200, 200);
#if defined(_3DS) || defined(_3DS_MOCK)
    qrTexture->setSize(100, 100);
#endif

    qrTexture->constrain(ALIGN_CENTER_HORIZONTAL);
    con->add(std::move(qrTexture));

#if !defined(_3DS) && !defined(_3DS_MOCK)
    con->height += 10;
#endif

    auto qrText = con->createNode<TextElement>("Scan to share network setup!",
                                               font_size - font_mod);

    // center these after the container is fully built
    qrText->constrain(ALIGN_CENTER_HORIZONTAL);

  } else {
    if (wifiInfo.unsupported) {
      con->createNode<TextElement>("Unsupported platform!",
                                   font_size + font_mod)
          ->constrain(ALIGN_CENTER_HORIZONTAL);
      con->createNode<TextElement>(
             "This build will never return any WiFi info.", font_size)
          ->constrain(ALIGN_CENTER_HORIZONTAL);
    } else {
      con->createNode<TextElement>("Issue reading WiFi info!",
                                   font_size + font_mod)
          ->constrain(ALIGN_CENTER_HORIZONTAL);
      con->createNode<TextElement>(
             "This app only works on the current network.",
             font_size - font_mod)
          ->constrain(ALIGN_CENTER_HORIZONTAL);
      con->createNode<TextElement>("Please connect to a hotspot and retry.",
                                   font_size - font_mod)
          ->constrain(ALIGN_CENTER_HORIZONTAL);
    }
    con->width = SCREEN_WIDTH;
  }

  con->constrain(ALIGN_CENTER_BOTH);

  // put the quit button in the bottom right
  auto btnText = "Close App";
  auto btnMargin = 35;
#if defined(_3DS) || defined(_3DS_MOCK)
  btnText = "Quit";
  btnMargin = 5;
#endif
  auto quitBtn = display->createNode<Button>(btnText, B_BUTTON, true);
  quitBtn->setAction([]() { RootDisplay::mainDisplay->requestQuit(); });
  quitBtn->constrain(ALIGN_BOTTOM | ALIGN_RIGHT, btnMargin);

// if we're on 3ds, we have to offset the container to target only the bottom
// screen
#if defined(_3DS) || defined(_3DS_MOCK)
  con->y += 240 / 2 -
            10; // this works because the two screens are both 240 pixels tall
#endif

  display->canUseSelectToExit = true;
  auto *displayPtr = display.release();
  return displayPtr->mainLoop();
}
