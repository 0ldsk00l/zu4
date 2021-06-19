/*
 * $Id: shrine.cpp 3021 2012-03-18 11:31:48Z daniel_santos $
 */


#include <string>
#include <vector>

#include "u4.h"

#include "shrine.h"

#include "annotation.h"
#include "context.h"
#include "event.h"
#include "game.h"
#include "imagemgr.h"
#include "location.h"
#include "mapmgr.h"
#include "creature.h"
#include "music.h"
#include "names.h"
#include "player.h"
#include "portal.h"
#include "screen.h"
#include "settings.h"
#include "tileset.h"
#include "types.h"

using std::string;
using std::vector;

int cycles, completedCycles;
static char *shrineAdvice[24];

/**
 * Returns true if the player can use the portal to the shrine
 */ 
bool shrineCanEnter(const Portal *p) {
    Shrine *shrine = dynamic_cast<Shrine*>(mapMgr->get(p->destid));
    if (!c->party->canEnterShrine(shrine->getVirtue())) {
        screenMessage("Thou dost not bear the rune of entry!  A strange force keeps you out!\n");
        return 0;
    }
    return 1;
}

/**
 * Returns true if 'map' points to a Shrine map
 */ 
bool isShrine(Map *punknown) {
    Shrine *ps;
    if ((ps = dynamic_cast<Shrine*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * Shrine class implementation
 */ 
Shrine::Shrine() {}

string Shrine::getName() {
    if (name.empty()) {
        name = "Shrine of ";
        name += getVirtueName(virtue);
    }
    return name;
}
Virtue Shrine::getVirtue() const    { return virtue; }
string Shrine::getMantra() const    { return mantra; }

void Shrine::setVirtue(Virtue v)    { virtue = v; }
void Shrine::setMantra(string m)    { mantra = m; }

/**
 * Enter the shrine
 */
void Shrine::enter() {

    U4FILE *avatar = u4fopen("avatar.exe");
    if (!avatar)
        return;
    zu4_read_strtable(avatar, 93682, (char**)shrineAdvice, 24);
    u4fclose(avatar);

    if (settings.enhancements && settings.enhancementsOptions.u5shrines)
        enhancedSequence();
    else  
        screenMessage("You enter the ancient shrine and sit before the altar...");

    screenMessage("\nUpon which virtue dost thou meditate?\n");
    string virtue;
    virtue = ReadStringController::get(32, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);

    int choice;
    screenMessage("\n\nFor how many Cycles (0-3)? ");
    choice = ReadChoiceController::get("0123\015\033");

    if (choice == '\033' || choice == '\015')
        cycles = 0;
    else
        cycles = choice - '0';
    completedCycles = 0;

    screenMessage("\n\n");

    // ensure the player chose the right virtue and entered a valid number for cycles
    if (strncasecmp(virtue.c_str(), getVirtueName(getVirtue()), 6) != 0 || cycles == 0) {
        screenMessage("Thou art unable to focus thy thoughts on this subject!\n");
        eject();
        return;
    }

    if (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) >= 0x10000) || (((c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff) != c->saveGame->lastmeditation)) {
        screenMessage("Begin Meditation\n");
        meditationCycle();
    }
    else { 
        screenMessage("Thy mind is still weary from thy last Meditation!\n");
        eject();
    }
}

void Shrine::enhancedSequence() {
    /* replace the 'static' avatar tile with grass */        
    annotations->add((Coords){5, 6, c->location->coords.z}, tileset->getByName("grass")->getId(), false, true);

    screenDisableCursor();
    screenMessage("You approach\nthe ancient\nshrine...\n");
    gameUpdateScreen(); EventHandler::wait_cycles(settings.gameCyclesPerSecond);
        
    Object *obj = addCreature(creatureMgr->getById(BEGGAR_ID), (Coords){5, 10, c->location->coords.z});
    obj->setTile(tileset->getByName("avatar")->getId());

    gameUpdateScreen(); EventHandler::wait_msecs(400);        
    c->location->map->move(obj, DIR_NORTH); gameUpdateScreen(); EventHandler::wait_msecs(400);
    c->location->map->move(obj, DIR_NORTH); gameUpdateScreen(); EventHandler::wait_msecs(400);
    c->location->map->move(obj, DIR_NORTH); gameUpdateScreen(); EventHandler::wait_msecs(400);
    c->location->map->move(obj, DIR_NORTH); gameUpdateScreen(); EventHandler::wait_msecs(800);
    obj->setTile(creatureMgr->getById(BEGGAR_ID)->getTile());
    gameUpdateScreen();
        
    screenMessage("\n...and kneel before the altar.\n");        
    EventHandler::wait_cycles(settings.gameCyclesPerSecond);
    screenEnableCursor();
}

void Shrine::meditationCycle() {
    /* find our interval for meditation */
    int interval = (settings.shrineTime * 1000) / MEDITATION_MANTRAS_PER_CYCLE;
    interval -= (interval % eventTimerGranularity);
    interval /= eventTimerGranularity;
    if (interval <= 0)
        interval = 1;    

    c->saveGame->lastmeditation = (c->saveGame->moves / SHRINE_MEDITATION_INTERVAL) & 0xffff;

    screenDisableCursor();
    for (int i = 0; i < MEDITATION_MANTRAS_PER_CYCLE; i++) {
        WaitController controller(interval);
        eventHandler->pushController(&controller);
        controller.wait();
        screenMessage(".");
    }
    askMantra();
}

void Shrine::askMantra() {
    screenEnableCursor();
    screenMessage("\nMantra: ");
    string mantra;
    mantra = ReadStringController::get(4, TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
    screenMessage("\n");

    if (strcasecmp(mantra.c_str(), getMantra().c_str()) != 0) {
        c->party->adjustKarma(KA_BAD_MANTRA);
        screenMessage("Thou art not able to focus thy thoughts with that Mantra!\n");
        eject();
    }
    else if (--cycles > 0) {
        completedCycles++;
        c->party->adjustKarma(KA_MEDITATION);
        meditationCycle();
    }
    else {
        completedCycles++;
        c->party->adjustKarma(KA_MEDITATION);

        bool elevated = completedCycles == 3 && c->party->attemptElevation(getVirtue());
        if (elevated)
            screenMessage("\nThou hast achieved partial Avatarhood in the Virtue of %s\n\n",
                          getVirtueName(getVirtue()));
        else
            screenMessage("\nThy thoughts are pure. "
                          "Thou art granted a vision!\n");

        ReadChoiceController::get("");
        showVision(elevated);
        ReadChoiceController::get("");
        gameSetViewMode(VIEW_NORMAL);
        eject();
    }
}

void Shrine::showVision(bool elevated) {
    static const char *visionImageNames[] = {
        BKGD_SHRINE_HON, BKGD_SHRINE_COM, BKGD_SHRINE_VAL, BKGD_SHRINE_JUS, 
        BKGD_SHRINE_SAC, BKGD_SHRINE_HNR, BKGD_SHRINE_SPI, BKGD_SHRINE_HUM
    };

    if (elevated) {
        screenMessage("Thou art granted a vision!\n");
        gameSetViewMode(VIEW_RUNE);
        screenDrawImageInMapArea(visionImageNames[getVirtue()]);
    }
    else {
        screenMessage("\n%s", shrineAdvice[getVirtue() * 3 + completedCycles - 1]);
    }
}

void Shrine::eject() {
    for (int i = 0; i < 24; i++) { free(shrineAdvice[i]); }
    game->exitToParentMap();
    zu4_music_play(c->location->map->music);
    c->location->turnCompleter->finishTurn();
}
