## ShoWiFi
Display active wifi info for homebrew'd Wii U and Switch consoles. 3DS and Wii support is partially implemented.

It also displays a [Wifi QR code](https://en.wikipedia.org/wiki/QR_code#Joining_a_Wi%E2%80%91Fi_network) to easily scan and share with other devices! 

### License
This app uses the Chesto library under [GPLv3](https://github.com/fortheusers/chesto/blob/master/LICENSE).

### Thanks
- @DanielKO and [libqrencode](https://github.com/fukuchi/libqrencode) contributors for qr code support
- Wii: [WiiBrew wiki](https://wiibrew.org/wiki//shared2/sys/net/02/config.dat) and contributors for documenting the config.dat file format.
- Wii U: @quarktheawesome for this [networking example](https://gbatemp.net/threads/simple-iosu-communication.432165/post-6483501) on Wii U.
- 3DS: @LiquidFenrir for [WiFiManager](https://github.com/LiquidFenrir/WifiManager/), which provided the structs used to read network auth info.
- Switch: @masagrator who has a similar app [GetNetworkPass](https://github.com/masagrator/GetNetworkPass/), and also provided info about sizes and services.

The efforts of all wiibrew, wiiubrew, 3dbrew, and switchbrew contributors, the authors of libogc, libcrtu, wut, and libnx, and the devkitPro organization.
