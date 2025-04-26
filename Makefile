BINARY      := ShoWiFi

APP_TITLE	:= ShoWiFi
APP_AUTHOR 	:= vgmoose
APP_VERSION := 1.1.1

# for some reason, the libqrencode library contains a main .c file, so we need to remove it
$(shell rm -f ./libs/libqrencode/qrenc.c || true)

SOURCES			+= . ./libs/libqrencode

# flags used by the libqrencode library
CFLAGS			+= -DSTATIC_IN_RELEASE=static -DVERSION="\"$(APP_VERSION)\"" -DMAJOR_VERSION=1 -DMINOR_VERSION=1 -DMICRO_VERSION=0

# for testing wii/3ds screen sizes
# CFLAGS 		+= -D_3DS_MOCK
# CFLAGS		+= -DWII_MOCK

include libs/chesto/Makefile
