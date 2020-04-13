#ifndef MUSIC_H
#define MUSIC_H

#ifdef __cplusplus
extern "C" {
#endif

#define CAMP_FADE_OUT_TIME 1000
#define CAMP_FADE_IN_TIME 0
#define INN_FADE_OUT_TIME 1000
#define INN_FADE_IN_TIME 5000

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

void zu4_music_play(int);
void zu4_music_stop();
void zu4_music_fadeout(int);
void zu4_music_fadein(int, bool);
void zu4_music_vol(double);
int zu4_music_vol_dec();
int zu4_music_vol_inc();
bool zu4_music_toggle();
void zu4_music_init();
void zu4_music_deinit();

#ifdef __cplusplus
}
#endif

#endif
