#ifndef SOUND_H
#define SOUND_H

#ifdef __cplusplus
extern "C" {
#endif

enum SoundEffect {
	SOUND_TITLE_FADE,       // the intro title fade
	SOUND_WALK_NORMAL_1,    // walk, world and town
	SOUND_WALK_NORMAL_2,    // walk, world and town
	SOUND_WALK_NORMAL_3,    // walk, world and town
	SOUND_WALK_NORMAL_4,    // walk, world and town
	SOUND_HORSE_NORMAL_1,   // walk, world and town
	SOUND_HORSE_NORMAL_2,   // walk, world and town
	SOUND_HORSE_NORMAL_3,   // walk, world and town
	SOUND_HORSE_NORMAL_4,   // walk, world and town
	SOUND_WALK_SLOWED,      // walk, slow progress
	SOUND_WALK_COMBAT,      // walk, combat
	SOUND_BLOCKED,          // location blocked
	SOUND_ERROR,            // error/bad command
	SOUND_PC_ATTACK,        // PC attacks
	SOUND_PC_STRUCK,        // PC damaged
	SOUND_NPC_ATTACK,       // NPC attacks
	SOUND_NPC_STRUCK,       // NPC damaged
	SOUND_ACID,             // effect, acid damage
	SOUND_SLEEP,            // effect, sleep
	SOUND_POISON_EFFECT,    // effect, poison
	SOUND_POISON_DAMAGE,    // damage, poison
	SOUND_EVADE,            // trap evaded
	SOUND_FLEE,             // flee combat
	SOUND_ITEM_STOLEN,      // item was stolen from a PC, food or gold
	SOUND_LBHEAL,           // LB heals party
	SOUND_LBRESURRECT,      // LB resurrects
	SOUND_HEALER,           // Healer heals or resurrects
	SOUND_LEVELUP,          // PC level up
	SOUND_MOONGATE,         // moongate used
	SOUND_CANNON,
	SOUND_RUMBLE,
	SOUND_PREMAGIC_MANA_JUMBLE,
	//	SOUND_PREAMBLE,
	SOUND_MAGIC,
	SOUND_AWAKE,
	SOUND_BLINK,
	SOUND_CURE,
	SOUND_DISPELL,
	SOUND_ENERGY_POISON,
	SOUND_ENERGY_SLEEP,
	SOUND_ENERGY_FIRE,
	SOUND_ENERGY_LIGHTNING,
	SOUND_FIREBALL,
	SOUND_GATE,
	SOUND_HEAL,
	SOUND_ICEBALL,
	SOUND_JINX,
	SOUND_KILL,
	SOUND_LIGHT,
	SOUND_MISSILE,
	SOUND_NEGATE,
	SOUND_OPEN,
	SOUND_PROTECTION,
	SOUND_QUICKNESS,
	SOUND_RESURRECT,
	SOUND_SLEEP_SPELL,
	SOUND_TREMOR,
	SOUND_UNDEAD,
	SOUND_VIEW,
	SOUND_WIND,
	SOUND_XIT,
	SOUND_YUP,
	SOUND_ZDOWN,
	SOUND_FAILED,
	SOUND_WHIRLPOOL,
	SOUND_STORM,
	//	SOUND_MISSED,
	//	SOUND_CREATUREATTACK,
	//	SOUND_PLAYERHIT,
	SOUND_MAX
};

void xu4_snd_play(int sound, bool onlyOnce, int specificDurationInTicks); // true, -1

void xu4_snd_stop();
void xu4_snd_vol(double);
int xu4_snd_vol_dec();
int xu4_snd_vol_inc();

void xu4_snd_init();
void xu4_snd_deinit();

#ifdef __cplusplus
}
#endif

#endif
