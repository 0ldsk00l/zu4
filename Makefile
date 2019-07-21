#CC=clang
#CXX=clang++
prefix=/usr/local
bindir=$(prefix)/bin
libdir=$(prefix)/lib
datadir=$(prefix)/share

UILIBS=$(shell sdl-config --libs) -lSDL_mixer
UIFLAGS=$(shell sdl-config --cflags)

FEATURES=-DHAVE_BACKTRACE=1 -DHAVE_VARIADIC_MACROS=1
#DEBUGCXXFLAGS=-ggdb1 -rdynamic -g -O0 -fno-inline -fno-eliminate-unused-debug-types -gstabs -g3
CXXFLAGS=$(FEATURES) -Isrc -Wall $(UIFLAGS) $(shell xml2-config --cflags) -DVERSION=\"$(VERSION)\" $(DEBUGCXXFLAGS)
CFLAGS=$(CXXFLAGS)
LIBS=$(UILIBS) $(shell xml2-config --libs) -lpng -lz
INSTALL=install

ifeq ($(STATIC_GCC_LIBS),true)
	LDFLAGS+=-L. -static-libgcc
endif

MAIN=u4

VERSION=1.1-git

CSRCS=\
		src/lzw/hash.c \
		src/lzw/lzw.c \
		src/lzw/u4decode.c \
		src/error.c \
		src/io.c \
		src/rle.c \
		src/savegame.c \
		src/u4_sdl.c \
		src/unzip.c

CXXSRCS=\
		src/annotation.cpp \
		src/armor.cpp \
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
		src/debug.cpp \
		src/dialogueloader.cpp \
		src/dialogueloader_hw.cpp \
		src/dialogueloader_lb.cpp \
		src/dialogueloader_tlk.cpp \
		src/direction.cpp \
		src/dungeon.cpp \
		src/dungeonview.cpp \
		src/event.cpp \
		src/event_sdl.cpp \
		src/filesystem.cpp \
		src/game.cpp \
		src/image_sdl.cpp \
		src/imageloader.cpp \
		src/imageloader_png.cpp \
		src/imageloader_u4.cpp \
		src/imageloader_u5.cpp \
		src/imageloader_fmtowns.cpp \
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
		src/music.cpp \
		src/music_sdl.cpp \
		src/names.cpp \
		src/object.cpp \
		src/person.cpp \
		src/player.cpp \
		src/portal.cpp \
		src/progress_bar.cpp \
		src/scale.cpp \
		src/script.cpp \
		src/screen.cpp \
		src/screen_sdl.cpp \
		src/settings.cpp \
		src/shrine.cpp \
		src/sound.cpp \
		src/sound_sdl.cpp \
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
		src/weapon.cpp \
		src/xml.cpp \
		src/lzw/u6decode.cpp

OBJS += $(CSRCS:.c=.o) $(CXXSRCS:.cpp=.o)

#all:: $(MAIN) mkutils
all:: $(MAIN)

mkutils::  coord dumpsavegame tlkconv u4dec u4enc u4unpackexe

$(MAIN): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

coord$(EXEEXT): util/coord.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+

dumpsavegame$(EXEEXT) : util/dumpsavegame.o savegame.o io.o names.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+

tlkconv : util/tlkconv.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+ $(shell xml2-config --libs)

u4dec : util/u4dec.o lzw/lzw.o lzw/u4decode.o lzw/hash.o rle.o util/pngconv.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+ -lpng -lz

u4enc : util/u4enc.o lzw/hash.o util/pngconv.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+ -lpng -lz

u4unpackexe: util/u4unpackexe.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+

#clean:: cleanutil
clean::
	rm -rf *~ */*~ $(OBJS) $(MAIN)

cleanutil::
	rm -rf util/coord.o coord util/dumpsavegame.o dumpsavegame util/u4dec.o u4dec util/u4enc.o u4enc util/pngconv.o util/tlkconv.o tlkconv util/u4unpackexe.o u4unpackexe

TAGS: $(CSRCS) $(CXXSRCS)
	etags *.h $(CSRCS) $(CXXSRCS)


install::
	$(INSTALL) -D $(MAIN) $(bindir)/$(MAIN)
	$(INSTALL) -D graphics/u4.png $(datadir)/pixmaps/u4.png
	mkdir -p $(libdir)/u4/music
	$(INSTALL) mid/*.mid $(libdir)/u4/music
	$(INSTALL) mid/*.it $(libdir)/u4/music
	mkdir -p $(libdir)/u4/sound
	$(INSTALL) sound/*.ogg $(libdir)/u4/sound
	$(INSTALL) -D coord $(libdir)/u4/coord
	$(INSTALL) -D dumpsavegame $(libdir)/u4/dumpsavegame
	$(INSTALL) -D tlkconv $(libdir)/u4/tlkconv
	$(INSTALL) -D u4dec $(libdir)/u4/u4dec
	$(INSTALL) -D u4enc $(libdir)/u4/u4enc
	$(INSTALL) -D u4unpackexe $(libdir)/u4/u4unpackexe
	$(INSTALL) conf/*.xml $(libdir)/u4
	mkdir -p $(libdir)/u4/dtd
	$(INSTALL) conf/dtd/*.dtd $(libdir)/u4/dtd
	mkdir -p $(libdir)/u4/graphics
	mkdir -p $(libdir)/u4/graphics/ega
	mkdir -p $(libdir)/u4/graphics/hires
	mkdir -p $(libdir)/u4/graphics/png
	mkdir -p $(libdir)/u4/graphics/vga
	mkdir -p $(libdir)/u4/graphics/vga2
	mkdir -p $(libdir)/u4/graphics/new
	$(INSTALL) graphics/ega/*.png $(libdir)/u4/graphics/ega
	$(INSTALL) graphics/hires/*.png $(libdir)/u4/graphics/hires
	$(INSTALL) graphics/hires/*.vga $(libdir)/u4/graphics/hires
	$(INSTALL) graphics/png/*.png $(libdir)/u4/graphics/png
	$(INSTALL) graphics/vga/*.png $(libdir)/u4/graphics/vga
	$(INSTALL) graphics/vga2/*.png $(libdir)/u4/graphics/vga2
	$(INSTALL) graphics/new/* $(libdir)/u4/graphics/new
	$(INSTALL) -D u4.desktop $(datadir)/applications/u4.desktop


# use GCC's automatic dependency generation
.depends::
	rm -f .depends
	$(CC) $(CFLAGS) -MM $(CSRCS) >> .depends
	$(CXX) $(CXXFLAGS) -MM $(CXXSRCS) >> .depends

# a target to build without dependencies on libstdc++, libgcc_s
all.static_gcc_libs:
	ln -s `$(CXX) -print-file-name=libstdc++.a` .
	$(MAKE) STATIC_GCC_LIBS=true
	rm -f libstdc++.a

-include .depends

