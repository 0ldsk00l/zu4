CC ?= cc
CXX ?= c++

CFLAGS ?= -O2
CXXFLAGS ?= -O2

FLAGS_C := -std=c99
FLAGS_CXX := -std=c++11

PREFIX ?= /usr/local
DATAROOTDIR ?= $(PREFIX)/share
DATADIR ?= $(DATAROOTDIR)
DOCDIR ?= $(DATAROOTDIR)/doc/$(NAME)
LIBDIR ?= $(PREFIX)/lib

USE_EXTERNAL_MINIZ ?= 0

PKG_CONFIG ?= pkg-config

CFLAGS_SDL2 := $(shell $(PKG_CONFIG) --cflags sdl2)
LIBS_SDL2 := $(shell $(PKG_CONFIG) --libs sdl2)

CFLAGS_XML2 := $(shell $(PKG_CONFIG) --cflags libxml-2.0)
LIBS_XML2 := $(shell $(PKG_CONFIG) --libs libxml-2.0)

#UIFLAGS += -ggdb1 -rdynamic -g -O0 -fno-inline -fno-eliminate-unused-debug-types -gstabs -g3

UIFLAGS := -Wall -Ideps/yxml $(CFLAGS_SDL2) $(CFLAGS_XML2)
UILIBS := $(LIBS_SDL2) $(LIBS_XML2) -lGL -lGLU

TARGET := u4

CSRCS := \
	deps/yxml/yxml.c \
	src/lzw/hash.c \
	src/lzw/lzw.c \
	src/lzw/u4decode.c \
	src/armor.c \
	src/cmixer.c \
	src/coords.c \
	src/direction.c \
	src/error.c \
	src/image.c \
	src/imageloader.c \
	src/io.c \
	src/moongate.c \
	src/music.c \
	src/names.c \
	src/random.c \
	src/rle.c \
	src/savegame.c \
	src/settings.c \
	src/sound.c \
	src/stb_vorbis.c \
	src/xmlparse.c \
	src/u4_sdl.c \
	src/video.c \
	src/weapon.c \
	src/u4file.c

CXXSRCS := \
	src/annotation.cpp \
	src/aura.cpp \
	src/camp.cpp \
	src/cheat.cpp \
	src/city.cpp \
	src/codex.cpp \
	src/combat.cpp \
	src/config.cpp \
	src/controller.cpp \
	src/context.cpp \
	src/conversation.cpp \
	src/creature.cpp \
	src/death.cpp \
	src/dialogueloader.cpp \
	src/dialogueloader_hw.cpp \
	src/dialogueloader_lb.cpp \
	src/dialogueloader_tlk.cpp \
	src/dungeon.cpp \
	src/dungeonview.cpp \
	src/event.cpp \
	src/event_sdl.cpp \
	src/game.cpp \
	src/imagemgr.cpp \
	src/imageview.cpp \
	src/intro.cpp \
	src/item.cpp \
	src/location.cpp \
	src/map.cpp \
	src/maploader.cpp \
	src/mapmgr.cpp \
	src/menu.cpp \
	src/menuitem.cpp \
	src/movement.cpp \
	src/object.cpp \
	src/person.cpp \
	src/player.cpp \
	src/portal.cpp \
	src/progress_bar.cpp \
	src/script.cpp \
	src/screen.cpp \
	src/shrine.cpp \
	src/spell.cpp \
	src/stats.cpp \
	src/textview.cpp \
	src/tile.cpp \
	src/tileanim.cpp \
	src/tilemap.cpp \
	src/tileset.cpp \
	src/tileview.cpp \
	src/u4.cpp \
	src/view.cpp \
	src/xml.cpp

ifeq ($(USE_EXTERNAL_MINIZ), 0)
	CFLAGS_MINIZ := -Ideps/miniz
	LIBS_MINIZ :=
	CSRCS += deps/miniz/miniz.c
else
	CFLAGS_MINIZ := $(shell $(PKG_CONFIG) --cflags miniz)
	LIBS_MINIZ := $(shell $(PKG_CONFIG) --libs miniz)
endif

UIFLAGS += $(CFLAGS_MINIZ)
UILIBS += $(LIBS_MINIZ)

.PHONY: all clean

OBJS := $(CSRCS:.c=.o) $(CXXSRCS:.cpp=.o)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(FLAGS_C) $(UIFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(FLAGS_CXX) $(UIFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $^ $(LDFLAGS) $(UILIBS) -o $@

clean:
	rm -rf *~ */*~ $(OBJS) $(TARGET)
