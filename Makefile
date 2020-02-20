CC := cc
CXX := c++
prefix := /usr/local
bindir = $(prefix)/bin
libdir = $(prefix)/lib
datadir = $(prefix)/share

UILIBS = $(shell sdl-config --libs) -lGL -lGLU
UIFLAGS = $(shell sdl-config --cflags)

CXXFLAGS = -Isrc -Wall $(UIFLAGS) $(shell xml2-config --cflags)
#CXXFLAGS += -ggdb1 -rdynamic -g -O0 -fno-inline -fno-eliminate-unused-debug-types -gstabs -g3
CFLAGS = $(CXXFLAGS)
LIBS = $(UILIBS) $(shell xml2-config --libs)

MAIN = u4

CSRCS=\
		src/lzw/hash.c \
		src/lzw/lzw.c \
		src/lzw/u4decode.c \
		src/armor.c \
		src/cmixer.c \
		src/coords.c \
		src/direction.c \
		src/error.c \
		src/image.c \
		src/io.c \
		src/miniz.c \
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
		src/yxml.c

CXXSRCS=\
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
		src/imageloader.cpp \
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
		src/moongate.cpp \
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
		src/u4file.cpp \
		src/utils.cpp \
		src/view.cpp \
		src/xml.cpp

OBJS += $(CSRCS:.c=.o) $(CXXSRCS:.cpp=.o)

all:: $(MAIN)

$(MAIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean::
	rm -rf *~ */*~ $(OBJS) $(MAIN)

TAGS: $(CSRCS) $(CXXSRCS)
	etags *.h $(CSRCS) $(CXXSRCS)
