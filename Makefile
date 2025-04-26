BINARY      := ShoWiFi

APP_TITLE	:= ShoWiFi
APP_AUTHOR 	:= vgmoose
APP_VERSION := 1.1.1

SOURCES			+= .

# from libs/libqrencode/Makefile.am
QRCODE_SOURCES := qrencode.c qrencode_inner.h \
				qrinput.c qrinput.h \
				bitstream.c bitstream.h \
				qrspec.c qrspec.h \
				rsecc.c rsecc.h \
				split.c split.h \
				mask.c mask.h \
				mqrspec.c mqrspec.h \
				mmask.c mmask.h

# get the full path to each of the source files, filtering only .c files
CFILES += $(foreach src,$(filter %.c,$(QRCODE_SOURCES)),$(CURDIR)/libs/libqrencode/$(src))

# flags used by the libqrencode library
CFLAGS			+= -DSTATIC_IN_RELEASE=static -DVERSION="\"$(APP_VERSION)\"" -DMAJOR_VERSION=1 -DMINOR_VERSION=1 -DMICRO_VERSION=0

# for testing wii/3ds screen sizes
# CFLAGS 		+= -D_3DS_MOCK
# CFLAGS		+= -DWII_MOCK

include libs/chesto/Makefile
