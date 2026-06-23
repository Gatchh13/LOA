#---------------------------------------------------------------------------------
# Legends of Aetheria - Nintendo 3DS Makefile
# Requires devkitARM + citro2d + citro3d (via devkitPro)
#
# Build:   make
# Clean:   make clean
# Install: make install  (copies .3dsx to SD card if DEVKITPRO_3DS is set)
#---------------------------------------------------------------------------------

.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "DEVKITPRO not set. Source /etc/profile.d/devkit-env.sh or set DEVKITPRO=/opt/devkitpro")
endif

TOPDIR        ?= $(CURDIR)

include $(DEVKITPRO)/rules/3ds.mk

#---------------------------------------------------------------------------------
# Target
#---------------------------------------------------------------------------------
TARGET        := LegendsOfAetheria
BUILD         := build
SOURCES       := source \
                 source/core \
                 source/input \
                 source/world \
                 source/render \
                 source/entities
INCLUDES      := include
DATA          :=
GRAPHICS      :=
ROMFS         := romfs

#---------------------------------------------------------------------------------
# App metadata (shown on 3DS home screen / CIA title)
#---------------------------------------------------------------------------------
APP_TITLE     := Legends of Aetheria
APP_DESCRIPTION:= Open World Fantasy RPG
APP_AUTHOR    := Solo Developer
ICON          := $(DEVKITPRO)/libctru/default_icon.png

#---------------------------------------------------------------------------------
# Compiler flags
#---------------------------------------------------------------------------------
ARCH          := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS        := -g -Wall -Wextra -O2 -mword-relocations \
                 -ffunction-sections -fdata-sections \
                 $(ARCH) $(INCLUDE) -DARM11 -D_3DS

CXXFLAGS      := $(CFLAGS) -std=c++17 -fno-rtti -fno-exceptions

ASFLAGS       := -g $(ARCH)

LDFLAGS       := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# Libraries
# Order matters: citro2d before citro3d before ctru
#---------------------------------------------------------------------------------
LIBS          := -lcitro2d -lcitro3d -lctru -lm

LIBDIRS       := $(CTRULIB) $(DEVKITPRO)/portlibs/3ds

#---------------------------------------------------------------------------------
# Source file discovery (auto-finds all .cpp and .c in SOURCES dirs)
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT   := $(CURDIR)/$(TARGET)
export TOPDIR   := $(CURDIR)

export VPATH    := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                   $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  := $(CURDIR)/$(BUILD)

CFILES          := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(CURDIR)/$(dir)/*.c)))
CPPFILES        := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(CURDIR)/$(dir)/*.cpp)))

export LD       := $(CXX)

export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export OFILES         := $(OFILES_SOURCES)
export HFILES_BIN     :=

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)

.PHONY: all clean install

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf

install: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@echo "Copy $(TARGET).3dsx to your 3DS SD card: /3ds/$(TARGET)/"

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
