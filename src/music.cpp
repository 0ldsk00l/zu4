/*
 * $Id: music.cpp 3019 2012-03-18 11:31:13Z daniel_santos $
 */
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_mixer.h>
#include "u4_sdl.h"

#include "music.h"
#include "sound.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4file.h"

using std::string;
using std::vector;

// C-like stuff
static char musicfiles[TRACK_MAX][128] = {{0}};
static int current;
static bool on = false;
static bool functional = true;

OSMusicMixer *playing = NULL;

static void xu4_music_create_sys() {
	// initialize sound subsystem
	int audio_rate = 44100;
	int audio_format = AUDIO_S16SYS;
	int audio_channels = 2;
	int audio_buffers = 1024;

	if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		xu4_error(XU4_LOG_WRN, "unable to init SDL audio subsystem: %s", SDL_GetError());
		functional = false;
		return;
	}

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		fprintf(stderr, "Unable to open audio!\n");
		functional = false;
		return;
	}
	functional = true;

	Mix_AllocateChannels(16);
}

static void xu4_music_destroy_sys() {
    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    Mix_CloseAudio();
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

static bool xu4_music_load_sys(const char *pathname) {
	if (playing) {
		Mix_FreeMusic(playing);
		playing = NULL;
	}

	playing = Mix_LoadMUS(pathname);
	if (!playing) {
		xu4_error(XU4_LOG_WRN, "unable to load music file %s: %s", pathname, Mix_GetError());
		return false;
	}
	return true;
}

bool xu4_music_load(int music) {
	ASSERT(music < TRACK_MAX, "Attempted to load an invalid piece of music in Music::load()");
	
	// music already loaded
	if (music == current) { return !xu4_music_playing(); }
	
	string pathname(u4find_music(musicfiles[music]));
	if (!pathname.empty()) {
		bool status = xu4_music_load_sys(pathname.c_str());
		if (status) { current = music; }
		return status;
	}
	return false;
}

bool xu4_music_functional() {
	return functional;
}

bool xu4_music_playing() {
	return Mix_PlayingMusic();
}

void xu4_music_play_track(int music) {
    if (!functional || !on) { return; }
	
    if (xu4_music_load(music)) {
        Mix_PlayMusic(playing, NLOOPS);
    }
}

void xu4_music_play() {
    xu4_music_play_track(c->location->map->music);
}

void xu4_music_stop() {
    Mix_HaltMusic();
}

void xu4_music_fadeout(int msecs) {
	// fade the music out even if 'on' is false
	if (!functional)
		return;

	if (xu4_music_playing()) {
		if (!settings.volumeFades)
			xu4_music_stop();
		else {
			if (Mix_FadeOutMusic(msecs) == -1) {
				xu4_error(XU4_LOG_WRN, "Mix_FadeOutMusic: %s\n", Mix_GetError());
			}
		}
	}
}

void xu4_music_fadein(int msecs, bool loadFromMap) {
	if (!functional || !on)
		return;

	if (!xu4_music_playing()) {
		/* make sure we've got something loaded to play */
		if (loadFromMap || !playing)
			xu4_music_load(c->location->map->music);

		if (!settings.volumeFades)
			xu4_music_play();
		else {
			if (Mix_FadeInMusic(playing, NLOOPS, msecs) == -1) {
				xu4_error(XU4_LOG_WRN, "Mix_FadeInMusic: %s\n", Mix_GetError());
			}
		}
	}
}

void xu4_music_vol(int volume) {
    Mix_VolumeMusic(int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

int xu4_music_vol_inc() {
    if (++settings.musicVol > MAX_VOLUME)
        settings.musicVol = MAX_VOLUME;
    else
        xu4_music_vol(settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

int xu4_music_vol_dec() {
    if (--settings.musicVol < 0)
        settings.musicVol = 0;
    else
        xu4_music_vol(settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

void xu4_snd_vol(int volume) {
    // Use Channel 1 for sound effects
    Mix_Volume(1, int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

int xu4_snd_vol_inc() {
    if (++settings.soundVol > MAX_VOLUME)
        settings.soundVol = MAX_VOLUME;
    else
        xu4_snd_vol(settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}

int xu4_snd_vol_dec() {
    if (--settings.soundVol < 0)
        settings.soundVol = 0;
    else
        xu4_snd_vol(settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}

// Ensures that the music is playing if it is supposed to be, or off
// if it is supposed to be turned off.
static void xu4_music_callback(void *data) {    
	eventHandler->getTimer()->remove(&xu4_music_callback);
	if (on && !xu4_music_playing()) { xu4_music_play(); }
	else if (!on && xu4_music_playing()) { xu4_music_stop(); }
}

// Toggle the music on/off (usually by pressing 'v')
bool xu4_music_toggle() {
    eventHandler->getTimer()->remove(&xu4_music_callback);
	
    on = !on;
    if (!on) {
        xu4_music_fadeout(1000);
	}
    else {
        xu4_music_fadein(1000, true);
	}
	
    eventHandler->getTimer()->add(&xu4_music_callback, settings.gameCyclesPerSecond);
    return on;    
}

// Initiliaze the music
void xu4_music_init() {
	current = TRACK_NONE;
	playing = NULL;
	
	// load music track filenames from xml config file
	const Config *config = Config::getInstance();
	
	vector<ConfigElement> musicConfs = config->getElement("music").getChildren();
	std::vector<ConfigElement>::const_iterator i = musicConfs.begin();
	std::vector<ConfigElement>::const_iterator theEnd = musicConfs.end();
	for (; i != theEnd; ++i) {
		if (i->getName() != "track") continue;
		int j = (i - musicConfs.begin()) + 1; // major hack while converting away from C++
		snprintf(musicfiles[j], sizeof(musicfiles[j]), "%s", i->getString("file").c_str());
	}
	
	xu4_music_create_sys(); // Call the Sound System specific creation file.
	
	// Set up the volume.
	on = settings.musicVol;
	xu4_music_vol(settings.musicVol);
	xu4_snd_vol(settings.soundVol);
}

void xu4_music_deinit() {
    eventHandler->getTimer()->remove(&xu4_music_callback);
	xu4_music_destroy_sys();
}
