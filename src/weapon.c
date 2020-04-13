/*
 * weapon.c
 * Copyright (C) 2020 R. Danbrook
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "weapon.h"

static weapon_t weapons[WEAP_MAX] = {
	{ WEAP_HANDS, "Hands", "HND", 0xff, 1, 8, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_STAFF, "Staff", "STF", 0xff, 1, 16, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_DAGGER, "Dagger", "DAG", 0xff, 10, 24, "hit_flash", "miss_flash", "", 0x2 },
	{ WEAP_SLING, "Sling", "SLN", 0xff, 10, 32, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_MACE, "Mace", "MAC", 0xfe, 1, 40, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_AXE, "Axe", "AXE", 0xf6, 1, 48, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_SWORD, "Sword", "SWD", 0xf6, 1, 64, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_BOW, "Bow", "BOW", 0x7e, 10, 40, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_CROSSBOW, "Crossbow", "XBO", 0x7e, 10, 56, "hit_flash", "miss_flash", "", 0x0 },
	{ WEAP_OIL, "Flaming Oil", "OIL", 0xff, 9, 64, "hit_flash", "miss_flash", "fire_field", 0x5 },
	{ WEAP_HALBERD, "Halberd", "HAL", 0x34, 2, 96, "hit_flash", "miss_flash", "", 0x2c0 },
	{ WEAP_MAGICAXE, "Magic Axe", "+AX", 0x30, 10, 96, "hit_flash", "miss_flash", "", 0x110 },
	{ WEAP_MAGICSWORD, "Magic Sword", "+SW", 0x74, 1, 128, "hit_flash", "miss_flash", "", 0x10 },
	{ WEAP_MAGICBOW, "Magic Bow", "+BO", 0x7a, 10, 80, "hit_flash", "miss_flash", "", 0x10 },
	{ WEAP_MAGICWAND, "Magic Wand", "WND", 0xb, 10, 160, "magic_flash", "magic_flash", "", 0x10 },
	{ WEAP_MYSTICSWORD, "Mystic Sword", "^SW", 0xff, 1, 255, "hit_flash", "miss_flash", "", 0x10 },
};

weapon_t* zu4_weapon(WeaponType type) {
	return &weapons[type];
}

const char* zu4_weapon_name(WeaponType type) {
	return weapons[type].name;
}

WeaponType zu4_weapon_type(const char* wname) {
	for (int i = WEAP_HANDS; i < WEAP_MAX; i++) {
		if (!strcasecmp(wname, weapons[i].name)) {
			return weapons[i].type;
		}
	}
	return WEAP_HANDS;
}

bool zu4_weapon_usable(WeaponType type, ClassType klass) {
	return weapons[type].usable & (1 << klass);
}
