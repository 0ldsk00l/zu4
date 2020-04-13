#ifndef WEAPON_H
#define WEAPON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "savegame.h"

enum Flags {
	WEAP_LOSE = 0x0001,
	WEAP_LOSEWHENRANGED = 0x0002,
	WEAP_CHOOSEDISTANCE = 0x0004,
	WEAP_ALWAYSHITS = 0x0008,
	WEAP_MAGIC = 0x0010,
	WEAP_ATTACKTHROUGHOBJECTS = 0x0040,
	WEAP_ABSOLUTERANGE = 0x0080,
	WEAP_RETURNS = 0x0100,
	WEAP_DONTSHOWTRAVEL = 0x0200,
};

typedef struct weapon_t {
	WeaponType type;
	const char *name;
	const char *abbr;
	uint8_t usable;
	int range;
	int damage;
	const char *hittile;
	const char *misstile;
	const char *leavetile;
	uint16_t flags;
} weapon_t;

weapon_t* zu4_weapon(WeaponType type);
const char* zu4_weapon_name(WeaponType type);
WeaponType zu4_weapon_type(const char* wname);
bool zu4_weapon_usable(WeaponType type, ClassType klass);

#ifdef __cplusplus
}
#endif

#endif
