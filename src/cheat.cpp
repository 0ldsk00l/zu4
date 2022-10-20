/*
 * $Id: cheat.cpp 3021 2012-03-18 11:31:48Z daniel_santos $
 */

#include "cheat.h"
#include "location.h"
#include "map.h"
#include "context.h"
#include "game.h"
#include "mapmgr.h"
#include "moongate.h"
#include "portal.h"
#include "player.h"
#include "screen.h"
#include "stats.h"
#include "tileset.h"
#include "utils.h"
#include "weapon.h"

static const char *strwhitespace = "\t\013\014 \n\r";

CheatMenuController::CheatMenuController(GameController *game) : game(game) {
}

bool CheatMenuController::keyPressed(int key) {
    int i;
    bool valid = true;

    switch (key) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        screenMessage("Gate %d!\n", key - '0');

        if (c->location->map->isWorldMap()) {
            const Coords *moongate = moongateGetGateCoordsForPhase(key - '1');
            if (moongate)
                c->location->coords = *moongate;
        }
        else
            screenMessage("Not here!\n");
        break;

    case 'a': {
        int newTrammelphase = c->saveGame->trammelphase + 1;
        if (newTrammelphase > 7)
            newTrammelphase = 0;

        screenMessage("Advance Moons!\n");
        while (c->saveGame->trammelphase != newTrammelphase)
            game->updateMoons(true);
        break;
    }

    case 'c':
        collisionOverride = !collisionOverride;
        screenMessage("Collision detection %s!\n", collisionOverride ? "off" : "on");
        break;

    case 'e':
        screenMessage("Equipment!\n");
        for (i = ARMR_NONE + 1; i < ARMR_MAX; i++)
            c->saveGame->armor[i] = 8;
        for (i = WEAP_HANDS + 1; i < WEAP_MAX; i++) {
            const weapon_t *weapon = zu4_weapon((WeaponType)i);
            if (weapon->flags & WEAP_LOSE || weapon->flags & WEAP_LOSEWHENRANGED)
                c->saveGame->weapons[i] = 99;
            else
                c->saveGame->weapons[i] = 8;
        }
        break;

    case 'f':
        screenMessage("Full Stats!\n");
        for (i = 0; i < c->saveGame->members; i++) {
            c->saveGame->players[i].str = 50;
            c->saveGame->players[i].dex = 50;
            c->saveGame->players[i].intel = 50;

            if (c->saveGame->players[i].hpMax < 800) {
                c->saveGame->players[i].xp = 9999;
                c->saveGame->players[i].hpMax = 800;
                c->saveGame->players[i].hp = 800;
            }
        }
        break;

    case 'g': {
        screenMessage("Goto: ");
        std::string dest = gameGetInput(32);
        transform(dest.begin(), dest.end(), dest.begin(), ::tolower);

        bool found = false;
        for (unsigned p = 0; p < c->location->map->portals.size(); p++) {
            MapId destid = c->location->map->portals[p]->destid;
            std::string destNameLower = mapMgr->get(destid)->getName();
            transform(destNameLower.begin(), destNameLower.end(), destNameLower.begin(), ::tolower);
            if (destNameLower.find(dest) != std::string::npos) {
                screenMessage("\n%s\n", mapMgr->get(destid)->getName().c_str());
                c->location->coords = c->location->map->portals[p]->coords;
                found = true;
                break;
            }
        }
        if (!found) {
            Coords coords = c->location->map->getLabel(dest);
            if (!zu4_coords_equal(coords, zu4_coords_nowhere())) {
                screenMessage("\n%s\n", dest.c_str());
                c->location->coords = coords;
                found = true;
            }
        }
        if (!found)
            screenMessage("\ncan't find\n%s!\n", dest.c_str());
        break;
    }

    case 'h': {
        screenMessage("Help:\n"
                      "1-8   - Gate\n"
                      "F1-F8 - +Virtue\n"
                      "a - Adv. Moons\n"
                      "c - Collision\n"
                      "e - Equipment\n"
                      "f - Full Stats\n"
                      "g - Goto\n"
                      "h - Help\n"
                      "i - Items\n"
                      "k - Show Karma\n"
                      "(more)");

        ReadChoiceController pauseController("");
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();

        screenMessage("\n"
                      "l - Location\n"
                      "m - Mixtures\n"
                      "o - Opacity\n"
                      "p - Peer\n"
                      "r - Reagents\n"
                      "s - Summon\n"
                      "t - Transports\n"
                      "v - Full Virtues\n"
                      "w - Change Wind\n"
                      "x - Exit Map\n"
                      "y - Y-up\n"
                      "(more)");

        eventHandler->pushController(&pauseController);
        pauseController.waitFor();

        screenMessage("\n"
                      "z - Z-down\n"
                  );
        break;
    }

    case 'i':
        screenMessage("Items!\n");
        c->saveGame->torches = 99;
        c->saveGame->gems = 99;
        c->saveGame->keys = 99;
        c->saveGame->sextants = 1;
        c->saveGame->items = ITEM_SKULL | ITEM_CANDLE | ITEM_BOOK | ITEM_BELL | ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T | ITEM_HORN | ITEM_WHEEL;
        c->saveGame->stones = 0xff;
        c->saveGame->runes = 0xff;
        c->saveGame->food = 999900;
        c->saveGame->gold = 9999;
        c->stats->update();
        break;

    case 'j':
        screenMessage("Joined by companions!\n");
        for (int m = c->saveGame->members; m < 8; m++) {
            fprintf(stderr, "m = %d\n", m);
            fprintf(stderr, "n = %s\n", c->saveGame->players[m].name);
            if (c->party->canPersonJoin(c->saveGame->players[m].name, NULL)) {
                c->party->join(c->saveGame->players[m].name);
            }
        }
        c->stats->update();
        break;

    case 'k':
        screenMessage("Karma!\n\n");
        for (i = 0; i < 8; i++) {
            unsigned int j;
            screenMessage("%s:", getVirtueName(static_cast<Virtue>(i)));
            for (j = 13; j > strlen(getVirtueName(static_cast<Virtue>(i))); j--)
                screenMessage(" ");
            if (c->saveGame->karma[i] > 0)
                screenMessage("%.2d\n", c->saveGame->karma[i]);
            else screenMessage("--\n");
        }
        break;

    case 'l':
        if (c->location->map->isWorldMap())
            screenMessage("\nLocation:\n%s\nx: %d\ny: %d\n", "World Map", c->location->coords.x, c->location->coords.y);
        else
            screenMessage("\nLocation:\n%s\nx: %d\ny: %d\nz: %d\n", c->location->map->getName().c_str(), c->location->coords.x, c->location->coords.y, c->location->coords.z);
        break;

    case 'm':
        screenMessage("Mixtures!\n");
        for (i = 0; i < SPELL_MAX; i++)
            c->saveGame->mixtures[i] = 99;
        break;

    case 'o':
        c->opacity = !c->opacity;
        screenMessage("Opacity %s!\n", c->opacity ? "on" : "off");
        break;

    case 'p':
        if ((c->location->viewMode == VIEW_NORMAL) || (c->location->viewMode == VIEW_DUNGEON))
            c->location->viewMode = VIEW_GEM;
        else if (c->location->context == CTX_DUNGEON)
            c->location->viewMode = VIEW_DUNGEON;
        else
            c->location->viewMode = VIEW_NORMAL;

        screenMessage("\nToggle View!\n");
        break;

    case 'r':
        screenMessage("Reagents!\n");
        for (i = 0; i < REAG_MAX; i++)
            c->saveGame->reagents[i] = 99;
        break;

    case 's':
        screenMessage("Summon!\n");
        screenMessage("What?\n");
        summonCreature(gameGetInput());
        break;

    case 't':
        if (c->location->map->isWorldMap()) {
            Coords coords = c->location->coords;
            static MapTile horse = c->location->map->tileset->getByName("horse")->getId(),
                ship = c->location->map->tileset->getByName("ship")->getId(),
                balloon = c->location->map->tileset->getByName("balloon")->getId();
            MapTile *choice;
            Tile *tile;

            screenMessage("Create transport!\nWhich? ");

            // Get the transport of choice
            char transport = ReadChoiceController::get("shb \033\015");
            switch(transport) {
                case 's': choice = &ship; break;
                case 'h': choice = &horse; break;
                case 'b': choice = &balloon; break;
                default:
                    choice = NULL;
                    break;
            }

            if (choice) {
                ReadDirController readDir;
                tile = c->location->map->tileset->get(choice->getId());

                screenMessage("%s\n", tile->getName().c_str());

                // Get the direction in which to create the transport
                eventHandler->pushController(&readDir);

                screenMessage("Dir: ");
                movedir(&coords, readDir.waitFor(), c->location->map);
                if (!zu4_coords_equal(coords, c->location->coords)) {
                    bool ok = false;
                    MapTile *ground = c->location->map->tileAt(coords, WITHOUT_OBJECTS);

                    screenMessage("%s\n", getDirectionName(readDir.getValue()));

                    switch(transport) {
                    case 's': ok = ground->getTileType()->isSailable(); break;
                    case 'h': ok = ground->getTileType()->isWalkable(); break;
                    case 'b': ok = ground->getTileType()->isWalkable(); break;
                    default: break;
                    }

                    if (choice && ok) {
                        c->location->map->addObject(*choice, *choice, coords);
                        screenMessage("%s created!\n", tile->getName().c_str());
                    }
                    else if (!choice)
                        screenMessage("Invalid transport!\n");
                    else screenMessage("Can't place %s there!\n", tile->getName().c_str());
                }
            }
            else screenMessage("None!\n");
        }
        break;

    case 'v':
        screenMessage("\nFull Virtues!\n");
        for (i = 0; i < 8; i++)
            c->saveGame->karma[i] = 0;
        c->stats->update();
        break;

    case 'w': {
        screenMessage("Wind Dir ('l' to lock):\n");
        WindCmdController ctrl;
        eventHandler->pushController(&ctrl);
        ctrl.waitFor();
        break;
    }

    case 'x':
        screenMessage("\nX-it!\n");
        if (!game->exitToParentMap())
            screenMessage("Not Here!\n");
        zu4_music_play(c->location->map->music);
        break;

    case 'y':
        screenMessage("Y-up!\n");
        if ((c->location->context & CTX_DUNGEON) && (c->location->coords.z > 0))
            c->location->coords.z--;
        else {
            screenMessage("Leaving...\n");
            game->exitToParentMap();
            zu4_music_play(c->location->map->music);
        }
        break;

    case 'z':
        screenMessage("Z-down!\n");
        if ((c->location->context & CTX_DUNGEON) && (c->location->coords.z < 7))
            c->location->coords.z++;
        else screenMessage("Not Here!\n");
        break;

    case U4_FKEY+0:
    case U4_FKEY+1:
    case U4_FKEY+2:
    case U4_FKEY+3:
    case U4_FKEY+4:
    case U4_FKEY+5:
    case U4_FKEY+6:
    case U4_FKEY+7:
        screenMessage("Improve %s!\n", getVirtueName(static_cast<Virtue>(key - U4_FKEY)));
        if (c->saveGame->karma[key - U4_FKEY] == 99)
            c->saveGame->karma[key - U4_FKEY] = 0;
        else if (c->saveGame->karma[key - U4_FKEY] != 0)
            c->saveGame->karma[key - U4_FKEY] += 10;
        if (c->saveGame->karma[key - U4_FKEY] > 99)
            c->saveGame->karma[key - U4_FKEY] = 99;
        c->stats->update();
        break;

    case U4_ESC:
    case U4_ENTER:
    case U4_SPACE:
        screenMessage("Nothing\n");
        break;

    default:
        valid = false;
        break;
    }

    if (valid) {
        doneWaiting();
        screenPrompt();
    }

    return valid;
}

/**
 * Summons a creature given by 'creatureName'. This can either be given
 * as the creature's name, or the creature's id.  Once it finds the
 * creature to be summoned, it calls gameSpawnCreature() to spawn it.
 */
void CheatMenuController::summonCreature(const std::string &name) {
    const Creature *m = NULL;
    std::string creatureName = name;

    creatureName.erase(creatureName.find_last_not_of(strwhitespace) + 1);
    creatureName.erase(0, creatureName.find_first_not_of(strwhitespace));

    if (creatureName.empty()) {
        screenMessage("\n");
        return;
    }

    /* find the creature by its id and spawn it */
    unsigned int id = atoi(creatureName.c_str());
    if (id > 0)
        m = creatureMgr->getById(id);

    if (!m)
        m = creatureMgr->getByName(creatureName);

    if (m) {
        if (gameSpawnCreature(m))
            screenMessage("\n%s summoned!\n", m->getName().c_str());
        else screenMessage("\n\nNo place to put %s!\n\n", m->getName().c_str());

        return;
    }

    screenMessage("\n%s not found\n", creatureName.c_str());
}

bool WindCmdController::keyPressed(int key) {
    switch (key) {
    case U4_UP:
    case U4_LEFT:
    case U4_DOWN:
    case U4_RIGHT:
        c->windDirection = keyToDirection(key);
        screenMessage("Wind %s!\n", getDirectionName(static_cast<Direction>(c->windDirection)));
        doneWaiting();
        return true;

    case 'l':
        c->windLock = !c->windLock;
        screenMessage("Wind direction is %slocked!\n", c->windLock ? "" : "un");
        doneWaiting();
        return true;
    }

    return KeyHandler::defaultHandler(key, NULL);
}
