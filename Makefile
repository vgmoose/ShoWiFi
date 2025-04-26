BINARY      := ShoWiFi

APP_TITLE	:= ShoWiFi
APP_AUTHOR 	:= vgmoose
APP_VERSION := 1.1

SOURCES		+= . libs/libqrencode

CFLAGS 			+= -DHAVE_PNG
CFLAGS			+= -Ilibs/libqrencode/src -DSTATIC_IN_RELEASE=static

# for testing wii/3ds screen sizes
# CFLAGS 		+= -D_3DS_MOCK
# CFLAGS		+= -DWII_MOCK

include libs/chesto/Makefile
