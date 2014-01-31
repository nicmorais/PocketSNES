
# Define the applications properties here:

TARGET = PocketSNES

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

ifdef V
	CMD:=
	SUM:=@\#
else
	CMD:=@
	SUM:=@echo
endif

INCLUDE = -I pocketsnes \
		-I sal/linux/include -I sal/include \
		-I pocketsnes/include -I pocketsnes/include/apu -I pocketsnes/include/unzip \
		-I pocketsnes/include/apu/bapu \
		-I pocketsnes/include/apu/bapu/dsp \
		-I pocketsnes/include/apu/bapu/smp \
		-I pocketsnes/include/apu/bapu/smp/core \
		-I pocketsnes/include/apu/bapu/smp/debugger \
		-I pocketsnes/include/apu/bapu/snes \
		-I menu -I pocketsnes/linux -I pocketsnes/snes9x

CFLAGS = $(INCLUDE) \
		 -g -O3 -pipe -ffast-math $(SDL_CFLAGS) \
		 -DRC_OPTIMIZED -D__LINUX__ -D__DINGUX__ -DHAVE_STDINT_H \
		 -DHAVE_STRINGS_H -DZLIB -DUNZIP_SUPPORT \
		 -flto

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

LDFLAGS = $(CXXFLAGS) -lpthread -lz -lpng -lm -lgcc $(SDL_LIBS)

# Find all source files
SOURCE = pocketsnes/snes9x pocketsnes/snes9x/apu \
		 pocketsnes/snes9x/apu/bapu/dsp \
		 pocketsnes/snes9x/apu/bapu/smp \
		 pocketsnes/snes9x/apu/bapu/smp/core \
		 pocketsnes/snes9x/apu/bapu/smp/debugger \
		 pocketsnes/snes9x/apu/bapu/dsp \
         menu sal/linux sal
SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c)) \
          pocketsnes/snes9x/unzip/zip.c \
          pocketsnes/snes9x/unzip/unzip.c \
          pocketsnes/snes9x/unzip/ioapi.c
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJS    = $(OBJ_CPP) $(OBJ_C)

.PHONY : all
all : $(TARGET)

.PHONY: opk
opk: $(TARGET).opk

$(TARGET) : $(OBJS)
	$(SUM) "  LD      $@"
	$(CMD)$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(TARGET).opk: $(TARGET)
	$(SUM) "  OPK     $@"
	$(CMD)rm -rf .opk_data
	$(CMD)cp -r data .opk_data
	$(CMD)cp $< .opk_data/pocketsnes.gcw0
	$(CMD)$(STRIP) .opk_data/pocketsnes.gcw0
	$(CMD)mksquashfs .opk_data $@ -all-root -noappend -no-exports -no-xattrs -no-progress >/dev/null

%.o: %.c
	$(SUM) "  CC      $@"
	$(CMD)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(SUM) "  CXX     $@"
	$(CMD)$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	$(SUM) "  CLEAN   ."
	$(CMD)rm -f $(OBJS) $(TARGET)
	$(CMD)rm -rf .opk_data $(TARGET).opk
