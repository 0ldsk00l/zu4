/*
 * music.c
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

#include "error.h"
#include "music.h"
#include "settings.h"
#include "sound.h"
#include "xmlparse.h"

static int curtrack = TRACK_NONE;
static int prevtrack = TRACK_NONE;
static bool music_enabled = false;

static SDL_AudioSpec *spec, *obtained;
static SDL_mutex *audio_mutex;

static cm_Source *track[TRACK_MAX];

static void lock_handler(cm_Event *e) {
	if (e->type == CM_EVENT_LOCK) {
		SDL_LockMutex(audio_mutex);
	}
	if (e->type == CM_EVENT_UNLOCK) {
		SDL_UnlockMutex(audio_mutex);
	}
}

void zu4_music_play(int music) {
	if (music_enabled) {
		if (curtrack == music) { return; }
		else {
			zu4_music_stop();
			curtrack = music;
			cm_play(track[curtrack]);
		}
	}
}

void zu4_music_stop() {
	if (curtrack && (cm_get_state(track[curtrack]) != CM_STATE_STOPPED)) {
		cm_stop(track[curtrack]);
	}
	prevtrack = curtrack;
	curtrack = TRACK_NONE;
}

void zu4_music_fadeout(int msecs) { // Implement later
	zu4_music_stop();
}

void zu4_music_fadein(int msecs, bool loadFromMap) { // Implement later
	zu4_music_play(prevtrack);
}

void zu4_music_vol(double volume) {
	// Every source has to be done independently
	for (int i = 1; i < TRACK_MAX; i++) {
		cm_set_gain(track[i], volume);
	}
}

int zu4_music_vol_inc() {
	if (++settings.musicVol > MAX_VOLUME) { settings.musicVol = MAX_VOLUME; }
	else { zu4_music_vol((double)settings.musicVol / MAX_VOLUME); }
	return (settings.musicVol * MAX_VOLUME);
}

int zu4_music_vol_dec() {
	if (--settings.musicVol < 0) { settings.musicVol = 0; }
	else { zu4_music_vol((double)settings.musicVol / MAX_VOLUME); }
	return (settings.musicVol * MAX_VOLUME);
}

bool zu4_music_toggle() {
	music_enabled = !music_enabled;
	music_enabled ? zu4_music_fadein(1000, true) : zu4_music_fadeout(1000);
	return music_enabled;
}

static void zu4_audio_cb(void *userdata, Uint8 *stream, int len) {
	cm_process((cm_Int16*)stream, len / 2);
}

static void zu4_music_free_files() {
	for (int i = 0; i < TRACK_MAX; i++) {
		if (track[i]) { free(track[i]); }
	}
}

static void zu4_music_load_files() {
	zu4_xmlparse_init("conf/music.xml");
	
	char trackfile[64]; // Buffer for track filenames that are found
	char trackpath[128]; // Buffer for full path to track
	int index = 1; // Start at 1 because track 0 is silence, and has no file
	while (zu4_xmlparse_find(trackfile, "track", "file")) {
		snprintf(trackpath, sizeof(trackpath), "%s%s", "music/", trackfile);
		track[index] = cm_new_source_from_file(trackpath);
		cm_set_loop(track[index++], 1);
	}
	
	zu4_xmlparse_deinit();
}

void zu4_music_init() {
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
	spec->callback = zu4_audio_cb;
	
	if (SDL_OpenAudio(spec, obtained) < 0) {
		zu4_error(ZU4_LOG_WRN, "Couldn't open audio: %s\n", SDL_GetError());
	}
	free(spec);
	
	cm_init(obtained->freq);
	cm_set_lock(lock_handler);
	
	zu4_music_load_files();
	music_enabled = settings.musicVol;
	
	zu4_music_vol((double)settings.musicVol / MAX_VOLUME);
	
	SDL_PauseAudio(0);
}

void zu4_music_deinit() {
	zu4_music_free_files();
	free(obtained);
	SDL_CloseAudio();
}
