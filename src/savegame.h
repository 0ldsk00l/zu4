#ifndef SAVEGAME_H
#define SAVEGAME_H

#include <stdio.h>

#define PARTY_SAV_BASE_FILENAME         "party.sav"
#define MONSTERS_SAV_BASE_FILENAME      "monsters.sav"
#define OUTMONST_SAV_BASE_FILENAME      "outmonst.sav"

#define MONSTERTABLE_SIZE                32
#define MONSTERTABLE_CREATURES_SIZE      8
#define MONSTERTABLE_OBJECTS_SIZE        (MONSTERTABLE_SIZE - MONSTERTABLE_CREATURES_SIZE)

/**
 * The list of all weapons.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
enum WeaponType {
	WEAP_HANDS,
	WEAP_STAFF, 
	WEAP_DAGGER,
	WEAP_SLING,
	WEAP_MACE,
	WEAP_AXE,
	WEAP_SWORD,
	WEAP_BOW,
	WEAP_CROSSBOW,
	WEAP_OIL,
	WEAP_HALBERD,
	WEAP_MAGICAXE,
	WEAP_MAGICSWORD,
	WEAP_MAGICBOW,
	WEAP_MAGICWAND,
	WEAP_MYSTICSWORD,
	WEAP_MAX
};

/**
 * The list of all armor types.  These values are used in both the
 * inventory fields and character records of the savegame.
 */
enum ArmorType {
	ARMR_NONE,
	ARMR_CLOTH,
	ARMR_LEATHER,
	ARMR_CHAIN,
	ARMR_PLATE,
	ARMR_MAGICCHAIN,
	ARMR_MAGICPLATE,
	ARMR_MYSTICROBES,
	ARMR_MAX
};

/**
 * The list of sex values for the savegame character records.  The
 * values match the male and female symbols in the character set.
 */
enum SexType {
	SEX_MALE = 0xb,
	SEX_FEMALE = 0xc
};

/**
 * The list of class types for the savegame character records.
 */
enum ClassType {
	CLASS_MAGE,
	CLASS_BARD,
	CLASS_FIGHTER,
	CLASS_DRUID,
	CLASS_TINKER,
	CLASS_PALADIN,
	CLASS_RANGER,
	CLASS_SHEPHERD
};

/**
 * The list of status values for the savegame character records.  The
 * values match the letter thats appear in the ztats area.
 */
enum StatusType {
	STAT_GOOD = 'G',
	STAT_POISONED = 'P',
	STAT_SLEEPING = 'S',
	STAT_DEAD = 'D'
};

enum Virtue {
	VIRT_HONESTY,
	VIRT_COMPASSION,
	VIRT_VALOR,
	VIRT_JUSTICE,
	VIRT_SACRIFICE,
	VIRT_HONOR,
	VIRT_SPIRITUALITY,
	VIRT_HUMILITY,
	VIRT_MAX
};

enum BaseVirtue {
	VIRT_NONE       = 0x00,
	VIRT_TRUTH      = 0x01,
	VIRT_LOVE       = 0x02,
	VIRT_COURAGE    = 0x04
};

enum Reagent {
	REAG_ASH,
	REAG_GINSENG,
	REAG_GARLIC,
	REAG_SILK,
	REAG_MOSS,
	REAG_PEARL,
	REAG_NIGHTSHADE,
	REAG_MANDRAKE,
	REAG_MAX
};

#define SPELL_MAX 26

enum Item {
	ITEM_SKULL  = 0x01,
	ITEM_SKULL_DESTROYED = 0x02,
	ITEM_CANDLE = 0x04,
	ITEM_BOOK   = 0x08,
	ITEM_BELL   = 0x10,
	ITEM_KEY_C  = 0x20,
	ITEM_KEY_L  = 0x40,
	ITEM_KEY_T  = 0x80,
	ITEM_HORN   = 0x100,
	ITEM_WHEEL  = 0x200,
	ITEM_CANDLE_USED = 0x400,
	ITEM_BOOK_USED = 0x800,
	ITEM_BELL_USED = 0x1000
};

enum Stone {
	STONE_BLUE   = 0x01,
	STONE_YELLOW = 0x02,
	STONE_RED    = 0x04,
	STONE_GREEN  = 0x08,
	STONE_ORANGE = 0x10,
	STONE_PURPLE = 0x20,
	STONE_WHITE  = 0x40,
	STONE_BLACK  = 0x80
};

enum Rune {
	RUNE_HONESTY      = 0x01,
	RUNE_COMPASSION   = 0x02,
	RUNE_VALOR        = 0x04,
	RUNE_JUSTICE      = 0x08,
	RUNE_SACRIFICE    = 0x10,
	RUNE_HONOR        = 0x20,
	RUNE_SPIRITUALITY = 0x40,
	RUNE_HUMILITY     = 0x80
};

/**
 * The Ultima IV savegame player record data.  
 */
typedef struct _SaveGamePlayerRecord {
	unsigned short hp;
	unsigned short hpMax;
	unsigned short xp;
	unsigned short str, dex, intel;
	unsigned short mp;
	unsigned short unknown;
	WeaponType weapon;
	ArmorType armor;
	char name[16];
	SexType sex;
	ClassType klass;
	StatusType status;
} SaveGamePlayerRecord;

/**
 * How Ultima IV stores monster information
 */
typedef struct _SaveGameMonsterRecord {
	unsigned char tile;
	unsigned char x;
	unsigned char y;
	unsigned char prevTile;
	unsigned char prevx;
	unsigned char prevy;
	unsigned char unused1;
	unsigned char unused2;
} SaveGameMonsterRecord;

/**
 * Represents the on-disk contents of PARTY.SAV.
 */
typedef struct SaveGame {
	unsigned int unknown1;
	unsigned int moves;
	SaveGamePlayerRecord players[8];
	int food;
	short gold;
	short karma[VIRT_MAX];
	short torches;
	short gems;
	short keys;
	short sextants;
	short armor[ARMR_MAX];
	short weapons[WEAP_MAX];
	short reagents[REAG_MAX];
	short mixtures[SPELL_MAX];
	unsigned short items;
	unsigned char x, y;
	unsigned char stones;
	unsigned char runes;
	unsigned short members;
	unsigned short transport;
	union {
		unsigned short balloonstate;
		unsigned short torchduration;
	};
	unsigned short trammelphase;
	unsigned short feluccaphase;
	unsigned short shiphull;
	unsigned short lbintro;
	unsigned short lastcamp;
	unsigned short lastreagent;
	unsigned short lastmeditation;
	unsigned short lastvirtue;
	unsigned char dngx, dngy;
	unsigned short orientation;
	unsigned short dnglevel;
	unsigned short location;
} SaveGame;

int saveGameMonstersWrite(SaveGameMonsterRecord *monsterTable, FILE *f);
int saveGameMonstersRead(SaveGameMonsterRecord *monsterTable, FILE *f);

void saveGameInit(SaveGame *sg, const SaveGamePlayerRecord *avatarInfo);
int saveGameRead(SaveGame *sg, FILE *f);
int saveGameWrite(SaveGame *sg, FILE *f);

void saveGamePlayerRecordInit(SaveGamePlayerRecord *player);
int saveGamePlayerRecordRead(SaveGamePlayerRecord *pRecord, FILE *f);
int saveGamePlayerRecordWrite(SaveGamePlayerRecord *pRecord, FILE *f);

#endif
