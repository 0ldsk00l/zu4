/*
 * savegame.c
 * Copyright (C) 2009 Andrew Taylor
 * Copyright (C) 2019 R. Danbrook
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
#include <string.h>

#include "io.h"
#include "savegame.h"

int saveGameWrite(SaveGame *sg, FILE *f) {
	// Write a SaveGame
	if (!writeInt(sg->unknown1, f) || !writeInt(sg->moves, f)) { return 0; }
	
	for (int i = 0; i < 8; i++) {
		if (!saveGamePlayerRecordWrite(&sg->players[i], f)) { return 0; }
	}
	
	if (!writeInt(sg->food, f) || !writeShort(sg->gold, f)) { return 0; }
	
	for (int i = 0; i < 8; i++) {
		if (!writeShort(sg->karma[i], f)) { return 0; }
	}
	
	if (!writeShort(sg->torches, f) || !writeShort(sg->gems, f) ||
		!writeShort(sg->keys, f) || !writeShort(sg->sextants, f)) {
		return 0;
	}
	
	for (int i = 0; i < ARMR_MAX; i++) {
		if (!writeShort(sg->armor[i], f)) { return 0; }
	}
	
	for (int i = 0; i < WEAP_MAX; i++) {
		if (!writeShort(sg->weapons[i], f)) { return 0; }
	}
	
	for (int i = 0; i < REAG_MAX; i++) {
		if (!writeShort(sg->reagents[i], f)) { return 0; }
	}
	
	for (int i = 0; i < SPELL_MAX; i++) {
		if (!writeShort(sg->mixtures[i], f)) { return 0; }
	}
	
	if (!writeShort(sg->items, f) ||
		!writeChar(sg->x, f) ||
		!writeChar(sg->y, f) ||
		!writeChar(sg->stones, f) ||
		!writeChar(sg->runes, f) ||
		!writeShort(sg->members, f) ||
		!writeShort(sg->transport, f) ||
		!writeShort(sg->balloonstate, f) ||
		!writeShort(sg->trammelphase, f) ||
		!writeShort(sg->feluccaphase, f) ||
		!writeShort(sg->shiphull, f) ||
		!writeShort(sg->lbintro, f) ||
		!writeShort(sg->lastcamp, f) ||
		!writeShort(sg->lastreagent, f) ||
		!writeShort(sg->lastmeditation, f) ||
		!writeShort(sg->lastvirtue, f) ||
		!writeChar(sg->dngx, f) ||
		!writeChar(sg->dngy, f) ||
		!writeShort(sg->orientation, f) ||
		!writeShort(sg->dnglevel, f) ||
		!writeShort(sg->location, f)) {
		return 0;
	}
	
	return 1;
}

int saveGameRead(SaveGame *sg, FILE *f) {
	// Read a SaveGame
	if (!readInt(&sg->unknown1, f) || !readInt(&sg->moves, f)) { return 0; }
	
	for (int i = 0; i < 8; i++) {
		//if (!sg->players[i].read(f))
		if (!saveGamePlayerRecordRead(&sg->players[i], f)) { return 0; }
	}
	
	if (!readInt((uint32_t*)&sg->food, f) ||
		!readShort((uint16_t*)&sg->gold, f)) {
		return 0;
	}
	
	for (int i = 0; i < 8; i++) {
		if (!readShort((uint16_t*)&sg->karma[i], f)) { return 0; }
	}
	
	if (!readShort((uint16_t*)&sg->torches, f) ||
		!readShort((uint16_t*)&sg->gems, f) ||
		!readShort((uint16_t*)&sg->keys, f) ||
		!readShort((uint16_t*)&sg->sextants, f)) {
		return 0;
	}
	
	for (int i = 0; i < ARMR_MAX; i++) {
		if (!readShort((uint16_t*)&sg->armor[i], f)) { return 0; }
	}
	
	for (int i = 0; i < WEAP_MAX; i++) {
		if (!readShort((uint16_t*)&sg->weapons[i], f)) { return 0; }
	}
	
	for (int i = 0; i < REAG_MAX; i++) {
		if (!readShort((uint16_t*)&sg->reagents[i], f)) { return 0; }
	}
	
	for (int i = 0; i < SPELL_MAX; i++) {
		if (!readShort((uint16_t*)&sg->mixtures[i], f)) { return 0; }
	}
	
	if (!readShort(&sg->items, f) ||
		!readChar(&sg->x, f) ||
		!readChar(&sg->y, f) ||
		!readChar(&sg->stones, f) ||
		!readChar(&sg->runes, f) ||
		!readShort(&sg->members, f) ||
		!readShort(&sg->transport, f) ||
		!readShort(&sg->balloonstate, f) ||
		!readShort(&sg->trammelphase, f) ||
		!readShort(&sg->feluccaphase, f) ||
		!readShort(&sg->shiphull, f) ||
		!readShort(&sg->lbintro, f) ||
		!readShort(&sg->lastcamp, f) ||
		!readShort(&sg->lastreagent, f) ||
		!readShort(&sg->lastmeditation, f) ||
		!readShort(&sg->lastvirtue, f) ||
		!readChar(&sg->dngx, f) ||
		!readChar(&sg->dngy, f) ||
		!readShort(&sg->orientation, f) ||
		!readShort(&sg->dnglevel, f) ||
		!readShort(&sg->location, f)) {
		return 0;
	}
	
	// workaround of U4DOS bug to retain savegame compatibility
	if (sg->location == 0 && sg->dnglevel == 0) {
		sg->dnglevel = 0xFFFF;
	}
	return 1;
}

void saveGameInit(SaveGame *sg, const SaveGamePlayerRecord *avatarInfo) {
	// Initialize a SaveGame
	sg->unknown1 = 0;
	sg->moves = 0;
	
	sg->players[0] = *avatarInfo;
	for (int i = 1; i < 8; i++) {
		saveGamePlayerRecordInit(&sg->players[i]);
	}
	
	sg->food = 0;
	sg->gold = 0;
	
	for (int i = 0; i < 8; i++) { sg->karma[i] = 20; }
	
	sg->torches = 0;
	sg->gems = 0;
	sg->keys = 0;
	sg->sextants = 0;
	
	for (int i = 0; i < ARMR_MAX; i++) { sg->armor[i] = 0; }
	for (int i = 0; i < WEAP_MAX; i++) { sg->weapons[i] = 0; }
	for (int i = 0; i < REAG_MAX; i++) { sg->reagents[i] = 0; }
	for (int i = 0; i < SPELL_MAX; i++) { sg->mixtures[i] = 0; }
	
	sg->items = 0;
	sg->x = 0;
	sg->y = 0;
	sg->stones = 0;
	sg->runes = 0;
	sg->members = 1;
	sg->transport = 0x1f;
	sg->balloonstate = 0;
	sg->trammelphase = 0;
	sg->feluccaphase = 0;
	sg->shiphull = 50;
	sg->lbintro = 0;
	sg->lastcamp = 0;
	sg->lastreagent = 0;
	sg->lastmeditation = 0;
	sg->lastvirtue = 0;
	sg->dngx = 0;
	sg->dngy = 0;
	sg->orientation = 0;
	sg->dnglevel = 0xFFFF;
	sg->location = 0;
}

int saveGamePlayerRecordWrite(SaveGamePlayerRecord *pRecord, FILE *f) {
	//Write a SaveGamePlayerRecord
	if (!writeShort(pRecord->hp, f) ||
		!writeShort(pRecord->hpMax, f) ||
		!writeShort(pRecord->xp, f) ||
		!writeShort(pRecord->str, f) ||
		!writeShort(pRecord->dex, f) ||
		!writeShort(pRecord->intel, f) ||
		!writeShort(pRecord->mp, f) ||
		!writeShort(pRecord->unknown, f) ||
		!writeShort((uint16_t)pRecord->weapon, f) ||
		!writeShort((uint16_t)pRecord->armor, f)) {
		return 0;
	}
	
	for (int i = 0; i < 16; i++) {
		if (!writeChar(pRecord->name[i], f)) { return 0; }
	}
	
	if (!writeChar((uint8_t)pRecord->sex, f) ||
		!writeChar((uint8_t)pRecord->klass, f) ||
		!writeChar((uint8_t)pRecord->status, f)) {
		return 0;
	}
	
	return 1;
}

int saveGamePlayerRecordRead(SaveGamePlayerRecord *pRecord, FILE *f) {
	// Read a SaveGamePlayerRecord
	uint8_t ch;
	uint16_t s;
	
	if (!readShort(&pRecord->hp, f) ||
		!readShort(&pRecord->hpMax, f) ||
		!readShort(&pRecord->xp, f) ||
		!readShort(&pRecord->str, f) ||
		!readShort(&pRecord->dex, f) ||
		!readShort(&pRecord->intel, f) ||
		!readShort(&pRecord->mp, f) ||
		!readShort(&pRecord->unknown, f)) {
		return 0;
	}
	
	if (!readShort(&s, f)) { return 0; }
	pRecord->weapon = (WeaponType)s;
	
	if (!readShort(&s, f)) {return 0; }
	pRecord->armor = (ArmorType)s;
	
	for (int i = 0; i < 16; i++) {
		if (!readChar((uint8_t *)&pRecord->name[i], f)) { return 0; }
	}

	if (!readChar(&ch, f)) { return 0; }
	pRecord->sex = (SexType)ch;
	
	if (!readChar(&ch, f)) { return 0; }
	pRecord->klass = (ClassType)ch;
	
	if (!readChar(&ch, f)) { return 0; }
	pRecord->status = (StatusType)ch;
	
	return 1;
}

void saveGamePlayerRecordInit(SaveGamePlayerRecord *pRecord) {
	// Initialize a SaveGamePlayerRecord
	pRecord->hp = 0;
	pRecord->hpMax = 0;
	pRecord->xp = 0;
	pRecord->str = 0;
	pRecord->dex = 0;
	pRecord->intel = 0;
	pRecord->mp = 0;
	pRecord->unknown = 0;
	pRecord->weapon = WEAP_HANDS;
	pRecord->armor = ARMR_NONE;
	
	for (int i = 0; i < 16; i++) { pRecord->name[i] = '\0'; }
	
	pRecord->sex = SEX_MALE;
	pRecord->klass = CLASS_MAGE;
	pRecord->status = STAT_GOOD;
}

int saveGameMonstersWrite(SaveGameMonsterRecord *monsterTable, FILE *f) {
	// Write a SaveGameMonsterRecord
	if (monsterTable) {
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].tile, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].x, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].y, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].prevTile, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].prevx, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].prevy, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].unused1, f)) { return 0; }
		}
		
		for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
			if (!writeChar(monsterTable[i].unused2, f)) { return 0; }
		}
	}
	else {
		int max = MONSTERTABLE_SIZE * 8;
		for (int i = 0; i < max; i++) {
			if (!writeChar((uint8_t)0, f)) { return 0; }
		}
	}
	return 1;
}

int saveGameMonstersRead(SaveGameMonsterRecord *monsterTable, FILE *f) {
	// Read a SaveGameMonsterRecord
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].tile, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].x, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].y, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].prevTile, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].prevx, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].prevy, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].unused1, f)) { return 0; }
	}
	
	for (int i = 0; i < MONSTERTABLE_SIZE; i++) {
		if (!readChar(&monsterTable[i].unused1, f)) { return 0; }
	}
	
	return 1;
}
#ifdef __cplusplus
}
#endif
