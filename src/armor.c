/*
 * armor.c
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "armor.h"

static armor_t armor[ARMR_MAX] = {
	{ ARMR_NONE, "Skin", 0xff, 96 },
	{ ARMR_CLOTH, "Cloth", 0xff, 128 },
	{ ARMR_LEATHER, "Leather", 0xfe, 144 },
	{ ARMR_CHAIN, "Chain Mail", 0x34, 160 },
	{ ARMR_PLATE, "Plate Mail", 0x34, 176 },
	{ ARMR_MAGICCHAIN, "Magic Chain", 0x24, 192 },
	{ ARMR_MAGICPLATE, "Magic Plate", 0x20, 208 },
	{ ARMR_MYSTICROBES, "Mystic Robe", 0xff, 248 },
};

armor_t* zu4_armor(ArmorType type) {
	return &armor[type];
}

int zu4_armor_defense(ArmorType type) {
	return armor[type].defense;
}

const char* zu4_armor_name(ArmorType type) {
	return armor[type].name;
}

ArmorType zu4_armor_type(const char* aname) {
	for (int i = ARMR_NONE; i < ARMR_MAX; i++) {
		if (!strcasecmp(aname, armor[i].name)) {
			return armor[i].type;
		}
	}
	return ARMR_NONE;
}

bool zu4_armor_wearable(ArmorType type, ClassType klass) {
	return armor[type].wearable & (1 << klass);
}
