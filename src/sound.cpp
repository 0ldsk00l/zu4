/*
 * $Id: sound.cpp 3019 2012-03-18 11:31:13Z daniel_santos $
 */

#include <SDL.h>
#include <SDL_mixer.h>

#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"

using std::string;
using std::vector;

static std::vector<Mix_Chunk *> soundChunk;
static char sndfiles[SOUND_MAX][128] = {{0}};

static bool load_sys(Sound sound, const char *pathname) {
    soundChunk[sound] = Mix_LoadWAV(pathname);
    if (!soundChunk[sound]) {
        xu4_error(XU4_LOG_WRN, "Unable to load sound effect file %s: %s", sndfiles[sound], Mix_GetError());
        return false;
    }
    return true;
}

static bool load(Sound sound) {
    ASSERT(sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()");
    // If music didn't initialize correctly, then we can't play it anyway
    
    if (!xu4_music_functional() || !settings.soundVol)
        return false;
    
    if (soundChunk[sound] == NULL) {
        string pathname(u4find_sound(sndfiles[sound]));
        string basename = pathname.substr(pathname.find_last_of("/") + 1);
        if (!basename.empty()) {
			return load_sys(sound, pathname.c_str());
		}
    }
    return true;    
}

int soundInit() {
	// load sound track filenames from xml config file
	const Config *config = Config::getInstance();
	soundChunk.resize(SOUND_MAX, NULL);
	
	vector<ConfigElement> soundConfs = config->getElement("sound").getChildren();
	vector<ConfigElement>::const_iterator i = soundConfs.begin();
	vector<ConfigElement>::const_iterator theEnd = soundConfs.end();
	for (; i != theEnd; ++i) {
		if (i->getName() != "track") { continue; }
		int j = (i - soundConfs.begin()); // major hack while converting away from C++
		snprintf(sndfiles[j], sizeof(sndfiles[j]), "%s", i->getString("file").c_str());
	}
	return 1;
}

void soundDelete() {
	for (int i = 0; i < SOUND_MAX; i++) {
		if (soundChunk[i]) { Mix_FreeChunk(soundChunk[i]); }
	}
}

void soundPlay(Sound sound, bool onlyOnce, int specificDurationInTicks) {
	ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");
	
	// If music didn't initialize correctly, then we can't play it anyway
	if (!xu4_music_functional() || !settings.soundVol) { return; }
	
	if (soundChunk[sound] == NULL) {
		if (!load(sound)) { return; }
	}
	
	// Use Channel 1 for sound effects
	if (!onlyOnce || !Mix_Playing(1)) {
		if (Mix_PlayChannelTimed(1, soundChunk[sound], specificDurationInTicks == -1 ? 0 : -1, specificDurationInTicks) == -1) {
			xu4_error(XU4_LOG_WRN, "Error playing sound %d: %s\n", sound, Mix_GetError());
		}
	}
}

void soundStop(int channel) {
	// If music didn't initialize correctly, then we shouldn't try to stop it
	if (!xu4_music_functional() || !settings.soundVol) { return; }
	if (Mix_Playing(channel)) { Mix_HaltChannel(channel); }
}
