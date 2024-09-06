#if defined(WII) || defined(WII_MOCK)
// Wii Headers from https://wiibrew.org/wiki//shared2/sys/net/02/config.dat

#define WII_CONFIG_PATH "/shared2/sys/net/02/config.dat"
// there is a note that the above path changes on <= IOS10:
// https://wiibrew.org/wiki//shared2/sys/net/config.dat

#if defined(WII)
#include <gccore.h>
#else
#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#endif

struct _proxy
{
    u8 use_proxy;               // 0x00 -> no proxy;  0x01 -> proxy
    u8 use_proxy_userandpass;    // 0x00 -> don't use username and password;  0x01 -> use username and password
    u8 padding_1[2];             // 0x00
    u8 proxy_name[255];
    u8 padding_2;               // 0x00
    u16 proxy_port;             // 0-34463 range
    u8 proxy_username[32];
    u8 padding_3;               // 0x00
    u8 proxy_password[32];
} __attribute__((__packed__));

typedef struct _proxy proxy_t;

typedef struct _connection
{
    /*
     *  Settings common to both wired and wireless connections
     */
    u8 flags;           // Defined below.
    u8 padding_1[3];

    u8 ip[4];            // Wii IP Address
    u8 netmask[4];
    u8 gateway[4];
    u8 dns1[4];
    u8 dns2[4];
    u8 padding_2[2];

    u16 mtu;            //valid values are 0 and 576-1500 range
    u8 padding_3[8];    // 0x00 padding?

    proxy_t proxy_settings;
    u8 padding_4;       //0x00

    proxy_t proxy_settings_copy;    // Seems to be a duplicate of proxy_settings
    u8 padding_5[1297];             //0x00

    /*
     *  Wireless specific settings
     */
    u8 ssid[32];        // Access Point name.

    u8 padding_6;       // 0x00
    u8 ssid_length;     // length of ssid[] (AP name) in bytes.
    u8 padding_7[2];    // 0x00

    u8 padding_8;       // 0x00
    u8 encryption;      // (Probably) Encryption.  OPN: 0x00, WEP64: 0x01, WEP128: 0x02 WPA-PSK (TKIP): 0x04, WPA2-PSK (AES): 0x05, WPA-PSK (AES): 0x06
    u8 padding_9[2];    // 0x00

    u8 padding_10;      // 0x00
    u8 key_length;      // length of key[] (encryption key) in bytes.  0x00 for WEP64 and WEP128.
    u8 unknown;         // 0x00 or 0x01 toogled with a WPA-PSK (TKIP) and with a WEP entered with hex instead of ascii.
    u8 padding_11;      // 0x00

    u8 key[64];         // Encryption key.  For WEP, key is stored 4 times (20 bytes for WEP64 and 52 bytes for WEP128) then padded with 0x00.

    u8 padding_12[236]; // 0x00
} connection_t;

typedef struct _netconfig
{
    u32 version;       // 0x00
    u8 header4;        // 0x01  When there's at least one valid connection to the Internet.
    u8 header5;        // 0x00
    u8 linkTimeoutSec; // 0x07  Link timeout in seconds, usually 7.
    u8 padding;        // 0x00

    connection_t connection[3];
} netconfig_t;
#endif

#if defined(_3DS)
#include <3ds.h>

// 3DS Headers from https://github.com/LiquidFenrir/WifiManager
// MIT License - Copyright (c) 2018 LiquidFenrir
#define CFG_WIFI_BLKID (u32 )0x00080000
#define CFG_WIFI_SLOTS 3
#define CFG_WIFI_SLOT_SIZE (u32 )0xC00

typedef enum {
    WIFI_ENCRYPTION_OPEN = 0,
    WIFI_ENCRYPTION_WEP_64,
    WIFI_ENCRYPTION_WEP_128,
    WIFI_ENCRYPTION_WEP_152,
    WIFI_ENCRYPTION_WPA_PSK_TKIP,
    WIFI_ENCRYPTION_WPA_PSK_AES,
    WIFI_ENCRYPTION_WPA2_PSK_TKIP,
    WIFI_ENCRYPTION_WPA2_PSK_AES,

    WIFI_ENCRYPTION_AMOUNT,
} wifi_encryption_type;

typedef enum {
    WIFI_AOSS = 0,
    WIFI_WPS,
} wifi_multissid_type;

typedef struct {
    char SSID[0x20];
    u8 SSID_length;
    u8 encryption_type;
    u8 __padding1[2];
    char password_text[0x40];
    u8 password_key[0x20];
    u8 __padding2[0x20];
} __attribute__((packed)) sub_network_s;

typedef struct {
    u16 version;
    u16 crc_checksum;

    u8 network_enable;

    u8 editable_security;
    u8 __padding1[2];
    sub_network_s main_network;

    u8 multissid_enable;
    u8 multissid_type;
    u8 multissid_amount;
    u8 __padding2[1];
    sub_network_s multissid_networks[4];

    u8 enable_DHCP;
    u8 enable_autoDNS;
    u8 __padding3[2];

    u8 ip_address[4];
    u8 gateway_address[4];
    u8 subnet_mask[4];

    u8 dns_primary[4];
    u8 dns_secondary[4];

    u8 connection_test_success;
    u8 group_privacy_mode; //needs more info
    u8 __padding4[2];
    u8 ip_to_use[4];
    u8 AP_MAC_address[6];
    u8 channel;
    u8 __padding5[1];

    u8 enable_proxy;
    u8 proxy_auth;

    u16 port_number;
    char proxy_url[0x30];
    u8 __padding6[0x34];
    char proxy_username[0x20];
    char proxy_password[0x20];

    u16 UPnP_value;
    u16 MTU_value;

    u8 __padding7[0x7EC];
} __attribute__((packed)) wifi_slot_s;
#endif