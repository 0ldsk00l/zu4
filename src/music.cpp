/*
 * music.cpp
 * Copyright (C) 2012 Daniel Santos
 * Copyright (C) 2019-2020 R. Danbrook
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include <SDL.h>

#include "cmixer.h"

#include "music.h"
#include "sound.h"
#include "context.h" // Last C++ dependency
#include "error.h"
#include "settings.h"
#include "xmlparse.h"

using std::string;
using std::vector;

static int curtrack = TRACK_NONE;
static bool music_enabled = false;

static SDL_AudioSpec *spec, *obtained;
static SDL_mutex* audio_mutex;

static cm_Source *track[TRACK_MAX];

static void lock_handler(cm_Event *e) {
	if (e->type == CM_EVENT_LOCK) {
		SDL_LockMutex(audio_mutex);
	}
	if (e->type == CM_EVENT_UNLOCK) {
		SDL_UnlockMutex(audio_mutex);
	}
}

void xu4_music_play_track(int music) {
	if (music_enabled) {
		if (curtrack == music) { return; }
		else {
			xu4_music_stop();
			curtrack = music;
			cm_play(track[curtrack]);
		}
	}
}

void xu4_music_play() {
	xu4_music_play_track(c->location->map->music);
}

void xu4_music_stop() {
	if (curtrack && (cm_get_state(track[curtrack]) != CM_STATE_STOPPED)) {
		cm_stop(track[curtrack]);
	}
	curtrack = TRACK_NONE;
}

void xu4_music_fadeout(int msecs) { // Implement later
	xu4_music_stop();
}

void xu4_music_fadein(int msecs, bool loadFromMap) { // Implement later
	xu4_music_play();
}

void xu4_music_vol(double volume) {
	// Every source has to be done independently
	for (int i = 1; i < TRACK_MAX; i++) {
		cm_set_gain(track[i], volume);
	}
}

int xu4_music_vol_inc() {
	if (++settings.musicVol > MAX_VOLUME) { settings.musicVol = MAX_VOLUME; }
	else { xu4_music_vol((double)settings.musicVol / MAX_VOLUME); }
	return (settings.musicVol * MAX_VOLUME);
}

int xu4_music_vol_dec() {
	if (--settings.musicVol < 0) { settings.musicVol = 0; }
	else { xu4_music_vol((double)settings.musicVol / MAX_VOLUME); }
	return (settings.musicVol * MAX_VOLUME);
}

bool xu4_music_toggle() {
	music_enabled = !music_enabled;
	music_enabled ? xu4_music_fadein(1000, true) : xu4_music_fadeout(1000);
	return music_enabled;
}

static void xu4_audio_cb(void *userdata, Uint8 *stream, int len) {
	cm_process((cm_Int16*)stream, len / 2);
}

static void xu4_music_free_files() {
	for (int i = 0; i < TRACK_MAX; i++) {
		if (track[i]) { free(track[i]); }
	}
}

static void xu4_music_load_files() {
	xu4_xmlparse_init("conf/music.xml");
	
	char trackfile[64]; // Buffer for track filenames that are found
	char trackpath[128]; // Buffer for full path to track
	int index = 1; // Start at 1 because track 0 is silence, and has no file
	while (xu4_xmlparse_find(trackfile, "track", "file")) {
		snprintf(trackpath, sizeof(trackpath), "%s%s", "music/", trackfile);
		track[index] = cm_new_source_from_file(trackpath);
		cm_set_loop(track[index++], 1);
	}
	
	xu4_xmlparse_deinit();
}

void xu4_music_init() {
	SDL_InitSubSystem(SDL_INIT_AUDIO);
	audio_mutex = SDL_CreateMutex();
	spec = (SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));
	obtained = (SDL_AudioSpec*)malloc(sizeof(SDL_AudioSpec));
	
	spec->channels = 2;
	spec->freq = 44100;
	spec->format = AUDIO_S16SYS;
	spec->silence = 0;
	spec->samples = 512;
	spec->userdata = 0;
	spec->callback = xu4_audio_cb;
	
	if (SDL_OpenAudio(spec, obtained) < 0) {
		xu4_error(XU4_LOG_WRN, "Couldn't open audio: %s\n", SDL_GetError());
	}
	free(spec);
	
	cm_init(obtained->freq);
	cm_set_lock(lock_handler);
	
	xu4_music_load_files();
	music_enabled = settings.musicVol;
	
	xu4_music_vol((double)settings.musicVol / MAX_VOLUME);
	
	SDL_PauseAudio(0);
}

void xu4_music_deinit() {
	xu4_music_free_files();
	free(obtained);
	SDL_CloseAudio();
}
