#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#define MIN_SHAKE_INTERVAL              50

#define MAX_BATTLE_SPEED                10
#define MAX_KEY_DELAY                   1000
#define MAX_KEY_INTERVAL                100
#define MAX_CYCLES_PER_SECOND           20
#define MAX_SPELL_EFFECT_SPEED          10
#define MAX_CAMP_TIME                   10
#define MAX_INN_TIME                    10
#define MAX_SHRINE_TIME                 20
#define MAX_SHAKE_INTERVAL              200
#define MAX_VOLUME                      10

#define DEFAULT_SCALE                   2
#define DEFAULT_FULLSCREEN              0
#define DEFAULT_VIDEO_TYPE              0 // 0 = EGA, 1 = VGA
#define DEFAULT_GEM_LAYOUT              0 // 0 = Standard, 1 = Full Viewport
#define DEFAULT_LINEOFSIGHT             0 // 0 = DOS, 1 = Enhanced
#define DEFAULT_SCREEN_SHAKES           1
#define DEFAULT_GAMMA                   100
#define DEFAULT_MUSIC_VOLUME            10
#define DEFAULT_SOUND_VOLUME            10
#define DEFAULT_VOLUME_FADES            1
#define DEFAULT_SHORTCUT_COMMANDS       0
#define DEFAULT_KEY_DELAY               300
#define DEFAULT_KEY_INTERVAL            30
#define DEFAULT_FILTER_MOVE_MESSAGES    0
#define DEFAULT_BATTLE_SPEED            5
#define DEFAULT_ENHANCEMENTS            1
#define DEFAULT_CYCLES_PER_SECOND       4
#define DEFAULT_DEBUG                   0
#define DEFAULT_VALIDATE_XML            0
#define DEFAULT_SPELL_EFFECT_SPEED      10
#define DEFAULT_CAMP_TIME               10
#define DEFAULT_INN_TIME                8
#define DEFAULT_SHRINE_TIME             16
#define DEFAULT_SHAKE_INTERVAL          100
#define DEFAULT_BATTLE_DIFFICULTY       0 // 0 = Normal, 1 = Hard, 2 = Expert
#define DEFAULT_TITLE_SPEED_RANDOM      150
#define DEFAULT_TITLE_SPEED_OTHER       30

typedef struct SettingsEnhancementOptions {
    bool activePlayer;
    bool u5spellMixing;
    bool u5shrines;
    bool u5combat;
    bool slimeDivides;
    bool gazerSpawnsInsects;
    bool textColorization;
    bool c64chestTraps;    
    bool smartEnterKey;
    bool peerShowsObjects;
} SettingsEnhancementOptions;

typedef struct SettingsData {
    int                 battleSpeed;
    bool                campingAlwaysCombat;
    int                 campTime;
    bool                debug;
    bool                enhancements;
    SettingsEnhancementOptions enhancementsOptions;    
    bool                filterMoveMessages;
    bool                fullscreen;
    int                 gameCyclesPerSecond;
    bool                innAlwaysCombat;
    int                 innTime;
    int                 keydelay;
    int                 keyinterval;
    int                 musicVol;
    unsigned int        scale;
    bool                screenShakes;
    int                 gamma;
    int                 shakeInterval;
    bool                shortcutCommands;
    int                 shrineTime;
    int                 soundVol;
    int                 spellEffectSpeed;
    bool                validateXml;
    bool                volumeFades;
    int                 titleSpeedRandom;
    int                 titleSpeedOther;
    int                 videoType;
    int                 gemLayout;
    int                 lineOfSight;
    int                 battleDiff;
} SettingsData;

extern SettingsData settings;

typedef struct u4settings_t {
	char path[64];
	char filename[80];
} u4settings_t;

u4settings_t* zu4_settings_ptr();
void zu4_settings_init(bool useProfile, const char *profileName);
void zu4_settings_setdata(const SettingsData data);
bool zu4_settings_read();
bool zu4_settings_write();

#ifdef __cplusplus
}
#endif

#endif
