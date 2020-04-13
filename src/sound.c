/*
 * sound.c
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

#include <stdio.h>
#include <stdlib.h>

#include "cmixer.h"

#include "error.h"
#include "settings.h"
#include "sound.h"
#include "xmlparse.h"

static cm_Source *effect[SOUND_MAX];

void zu4_snd_play(int sound, bool onlyOnce, int specificDurationInTicks) {
	if (sound >= SOUND_MAX) {
		zu4_error(ZU4_LOG_WRN, "Tried to play an invalid sound!");
		return;
	}
	else if (!settings.soundVol) { return; }
	else if (effect[sound] == NULL) { return; }
	
	cm_play(effect[sound]);
}

void zu4_snd_stop() {
	for (int i = 0; i < SOUND_MAX; i++) {
		cm_stop(effect[i]);
	}
}

void zu4_snd_vol(double volume) {
	// Every source has to be done independently
    for (int i = 0; i < SOUND_MAX; i++) {
		cm_set_gain(effect[i], volume);
	}
}

int zu4_snd_vol_inc() {
	if (++settings.soundVol > MAX_VOLUME) { settings.soundVol = MAX_VOLUME; }
	else { zu4_snd_vol((double)settings.soundVol / MAX_VOLUME); }
	return (settings.soundVol * MAX_VOLUME);
}

int zu4_snd_vol_dec() {
	if (--settings.soundVol < 0) { settings.soundVol = 0; }
	else { zu4_snd_vol((double)settings.soundVol / MAX_VOLUME); }
	return (settings.soundVol * MAX_VOLUME);
}

static void zu4_snd_free_files() {
	for (int i = 0; i < SOUND_MAX; i++) {
		if (effect[i]) { free(effect[i]); }
	}
}

static void zu4_snd_load_files() {
	zu4_xmlparse_init("conf/sound.xml");
	
	char soundfile[64]; // Buffer for sound effect filenames that are found
	char soundpath[128]; // Buffer for full path to sound effect
	int index = 0;
	while (zu4_xmlparse_find(soundfile, "track", "file")) {
		snprintf(soundpath, sizeof(soundpath), "%s%s", "sound/", soundfile);
		effect[index++] = cm_new_source_from_file(soundpath);
	}
	
	zu4_xmlparse_deinit();
}

void zu4_snd_init() {
	zu4_snd_load_files();
	zu4_snd_vol((double)settings.soundVol / MAX_VOLUME);
}

void zu4_snd_deinit() {
	zu4_snd_free_files();
}
