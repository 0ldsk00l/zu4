#ifndef MUSIC_H
#define MUSIC_H

#define CAMP_FADE_OUT_TIME          1000
#define CAMP_FADE_IN_TIME           0
#define INN_FADE_OUT_TIME           1000
#define INN_FADE_IN_TIME            5000
#define NLOOPS -1

enum MusicTrack {
	TRACK_NONE,
	TRACK_OUTSIDE,
	TRACK_TOWNS,
	TRACK_SHRINES,
	TRACK_SHOPPING,
	TRACK_RULEBRIT,
	TRACK_FANFARE,
	TRACK_DUNGEON,
	TRACK_COMBAT,
	TRACK_CASTLES,
	TRACK_MAX
};

//bool xu4_music_load(int);
bool xu4_music_functional();
bool xu4_music_playing();
void xu4_music_play_track(int);
void xu4_music_play();
void xu4_music_stop();
void xu4_music_fadeout(int);
void xu4_music_fadein(int, bool);
void xu4_music_vol(int);
int xu4_music_vol_dec();
int xu4_music_vol_inc();
void xu4_snd_vol(int);
int xu4_snd_vol_dec();
int xu4_snd_vol_inc();
bool xu4_music_toggle();

void xu4_music_init();
void xu4_music_deinit();

// SDL
//struct _Mix_Music;
//typedef _Mix_Music OSMusicMixer;

#endif
