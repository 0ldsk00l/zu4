#ifndef ARMOR_H
#define ARMOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "savegame.h"

typedef struct armor_t {
	ArmorType type;      // Enumerated Armor Type
	const char *name;    // Name of the Armor
	uint8_t wearable;    // What classes can wear the Armor
	int defense;         // Defense value of the Armor
} armor_t;

armor_t* xu4_armor(ArmorType type);
int xu4_armor_defense(ArmorType type);
const char* xu4_armor_name(ArmorType type);
ArmorType xu4_armor_type(const char* aname);
bool xu4_armor_wearable(ArmorType type, ClassType klass);

#ifdef __cplusplus
}
#endif

#endif
