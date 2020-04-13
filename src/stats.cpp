/*
 * $Id: stats.cpp 3019 2012-03-18 11:31:13Z daniel_santos $
 */
#include <string.h>

#include "u4.h"
#include "stats.h"

#include "armor.h"
#include "context.h"
#include "error.h"
#include "menu.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "spell.h"
#include "tile.h"
#include "weapon.h"

extern bool verbose;

/**
 * StatsArea class implementation
 */
StatsArea::StatsArea() : 
    title(STATS_AREA_X * CHAR_WIDTH, 0 * CHAR_HEIGHT, STATS_AREA_WIDTH, 1),
    mainArea(STATS_AREA_X * CHAR_WIDTH, STATS_AREA_Y * CHAR_HEIGHT, STATS_AREA_WIDTH, STATS_AREA_HEIGHT),
    summary(STATS_AREA_X * CHAR_WIDTH, (STATS_AREA_Y + STATS_AREA_HEIGHT + 1) * CHAR_HEIGHT, STATS_AREA_WIDTH, 1),
    view(STATS_PARTY_OVERVIEW)
{
    // Generate a formatted string for each menu item,
    // and then add the item to the menu.  The Y value
    // for each menu item will be filled in later.
    for (int count=0; count < 8; count++)
    {
        char outputBuffer[16];
        snprintf(outputBuffer, sizeof(outputBuffer), "-%-11s%%s", getReagentName((Reagent)count));
        reagentsMixMenu.add(count, new IntMenuItem(outputBuffer, 1, 0, -1, (int *)c->party->getReagentPtr((Reagent)count), 0, 99, 1, MENU_OUTPUT_REAGENT));
    }

    reagentsMixMenu.addObserver(this);
}
 
void StatsArea::setView(StatsView view) {
    this->view = view;
    update();
}

/**
 * Sets the stats item to the previous in sequence.
 */
void StatsArea::prevItem() {
    view = (StatsView)(view - 1);
    if (view < STATS_CHAR1)
        view = STATS_MIXTURES;
    if (view <= STATS_CHAR8 && (view - STATS_CHAR1 + 1) > c->party->size())
        view = (StatsView) (STATS_CHAR1 - 1 + c->party->size());
    update();
}

/**
 * Sets the stats item to the next in sequence.
 */
void StatsArea::nextItem() {
    view = (StatsView)(view + 1);    
    if (view > STATS_MIXTURES)
        view = STATS_CHAR1;
    if (view <= STATS_CHAR8 && (view - STATS_CHAR1 + 1) > c->party->size())
        view = STATS_WEAPONS;
    update();
}

/**
 * Update the stats (ztats) box on the upper right of the screen.
 */
void StatsArea::update(bool avatarOnly) {
    clear();

    /*
     * update the upper stats box
     */
    switch(view) {
    case STATS_PARTY_OVERVIEW:
        showPartyView(avatarOnly);
        break;
    case STATS_CHAR1:
    case STATS_CHAR2:
    case STATS_CHAR3:
    case STATS_CHAR4:
    case STATS_CHAR5:
    case STATS_CHAR6:
    case STATS_CHAR7:
    case STATS_CHAR8:
        showPlayerDetails();
        break;
    case STATS_WEAPONS:
        showWeapons();
        break;
    case STATS_ARMOR:
        showArmor();
        break;
    case STATS_EQUIPMENT:
        showEquipment();
        break;
    case STATS_ITEMS:
        showItems();
        break;
    case STATS_REAGENTS:
        showReagents();
        break;
    case STATS_MIXTURES:
        showMixtures();
        break;
    case MIX_REAGENTS:
        showReagents(true);
        break;
    }

    /*
     * update the lower stats box (food, gold, etc.)
     */
    if (c->transportContext == TRANSPORT_SHIP)
        summary.textAt(0, 0, "F:%04d   SHP:%02d", c->saveGame->food / 100, c->saveGame->shiphull);
    else
        summary.textAt(0, 0, "F:%04d   G:%04d", c->saveGame->food / 100, c->saveGame->gold);

    update(c->aura);

    redraw();
}

void StatsArea::update(Aura *aura) {
    unsigned char mask = 0xff;
    for (int i = 0; i < VIRT_MAX; i++) {
        if (c->saveGame->karma[i] == 0)
            mask &= ~(1 << i);
    }

    switch (aura->type) {
    case AURA_NONE:
        summary.drawCharMasked(0, STATS_AREA_WIDTH/2, 0, mask);
        break;
    case AURA_HORN:
        summary.drawChar(CHARSET_REDDOT, STATS_AREA_WIDTH/2, 0);
        break;
    case AURA_JINX:
        summary.drawChar('J', STATS_AREA_WIDTH/2, 0);
        break;
    case AURA_NEGATE:
        summary.drawChar('N', STATS_AREA_WIDTH/2, 0);
        break;
    case AURA_PROTECTION:
        summary.drawChar('P', STATS_AREA_WIDTH/2, 0);
        break;
    case AURA_QUICKNESS:
        summary.drawChar('Q', STATS_AREA_WIDTH/2, 0);
        break;
    }    

    summary.update();
}

void StatsArea::highlightPlayer(int player) {
    zu4_assert(player < c->party->size(), "player number out of range: %d", player);
    mainArea.highlight(0, player * CHAR_HEIGHT, STATS_AREA_WIDTH * CHAR_WIDTH, CHAR_HEIGHT);
}

void StatsArea::clear() {
    for (int i = 0; i < STATS_AREA_WIDTH; i++)
        title.drawChar(CHARSET_HORIZBAR, i, 0);

    mainArea.clear();
    summary.clear();
}

/**
 * Redraws the entire stats area
 */
void StatsArea::redraw() {
    title.update();
    mainArea.update();
    summary.update();
}

/**
 * Sets the title of the stats area.
 */
void StatsArea::setTitle(const string &s) {
    int titleStart = (STATS_AREA_WIDTH / 2) - ((s.length() + 2) / 2);
    title.textAt(titleStart, 0, "%c%s%c", 16, s.c_str(), 17);
}

/**
 * The basic party view.
 */
void StatsArea::showPartyView(bool avatarOnly) {
    const char *format = "%d%c%-9.8s%3d%s";

    PartyMember *p = NULL;
    int activePlayer = c->party->getActivePlayer();

    zu4_assert(c->party->size() <= 8, "party members out of range: %d", c->party->size());

    if (!avatarOnly) {
        for (int i = 0; i < c->party->size(); i++) {
            p = c->party->member(i);
            mainArea.textAt(0, i, format, i+1, (i==activePlayer) ? CHARSET_BULLET : '-', p->getName().c_str(), p->getHp(), mainArea.colorizeStatus(p->getStatus()).c_str());
        }
    }
    else {        
        p = c->party->member(0);
        mainArea.textAt(0, 0, format, 1, (activePlayer==0) ? CHARSET_BULLET : '-', p->getName().c_str(), p->getHp(), mainArea.colorizeStatus(p->getStatus()).c_str());
    }
}

/**
 * The individual character view.
 */
void StatsArea::showPlayerDetails() {
    int player = view - STATS_CHAR1;

    zu4_assert(player < 8, "character number out of range: %d", player);

    PartyMember *p = c->party->member(player);
    setTitle(p->getName());
    mainArea.textAt(0, 0, "%c             %c", p->getSex(), p->getStatus());
    string classStr = getClassName(p->getClass());
    int classStart = (STATS_AREA_WIDTH / 2) - (classStr.length() / 2);
    mainArea.textAt(classStart, 0, "%s", classStr.c_str());
    mainArea.textAt(0, 2, " MP:%02d  LV:%d", p->getMp(), p->getRealLevel());
    mainArea.textAt(0, 3, "STR:%02d  HP:%04d", p->getStr(), p->getHp());
    mainArea.textAt(0, 4, "DEX:%02d  HM:%04d", p->getDex(), p->getMaxHp());
    mainArea.textAt(0, 5, "INT:%02d  EX:%04d", p->getInt(), p->getExp());
    const weapon_t *w = p->getWeapon();
    mainArea.textAt(0, 6, "W:%s", w->name);
    const armor_t *a = p->getArmor();
    mainArea.textAt(0, 7, "A:%s", a->name);
}

/**
 * Weapons in inventory.
 */
void StatsArea::showWeapons() {
    setTitle("Weapons");

    int line = 0;
    int col = 0;
    mainArea.textAt(0, line++, "A-Hands");
    for (int w = WEAP_HANDS + 1; w < WEAP_MAX; w++) {
        int n = c->saveGame->weapons[w];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            const char *format = (n >= 10) ? "%c%d-%s" : "%c-%d-%s";
            weapon_t *weapon = zu4_weapon((WeaponType)w);
            mainArea.textAt(col, line++, format, w - WEAP_HANDS + 'A', n, weapon->abbr);
            if (line >= (STATS_AREA_HEIGHT)) {
                line = 0;
                col += 8;
            }
        }
    }    
}

/**
 * Armor in inventory.
 */
void StatsArea::showArmor() {
    setTitle("Armour");

    int line = 0;
    mainArea.textAt(0, line++, "A  -No Armour");
    for (int a = ARMR_NONE + 1; a < ARMR_MAX; a++) {
        if (c->saveGame->armor[a] > 0) {
            const char *format = (c->saveGame->armor[a] >= 10) ? "%c%d-%s" : "%c-%d-%s";
            
            mainArea.textAt(0, line++, format, a - ARMR_NONE + 'A', c->saveGame->armor[a], zu4_armor_name((ArmorType)a));
        }
    }
}

/**
 * Equipment: touches, gems, keys, and sextants.
 */
void StatsArea::showEquipment() {
    setTitle("Equipment");

    int line = 0;
    mainArea.textAt(0, line++, "%2d Torches", c->saveGame->torches);
    mainArea.textAt(0, line++, "%2d Gems", c->saveGame->gems);
    mainArea.textAt(0, line++, "%2d Keys", c->saveGame->keys);
    if (c->saveGame->sextants > 0)
        mainArea.textAt(0, line++, "%2d Sextants", c->saveGame->sextants);    
}

/**
 * Items: runes, stones, and other miscellaneous quest items.
 */
void StatsArea::showItems() {
    int i, j;
    char buffer[17];

    setTitle("Items");

    int line = 0;
    if (c->saveGame->stones != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->stones & (1 << i))
                buffer[j++] = getStoneName((Virtue) i)[0];
        }
        buffer[j] = '\0';
        mainArea.textAt(0, line++, "Stones:%s", buffer);
    }
    if (c->saveGame->runes != 0) {
        j = 0;
        for (i = 0; i < 8; i++) {
            if (c->saveGame->runes & (1 << i))
                buffer[j++] = getVirtueName((Virtue) i)[0];
        }
        buffer[j] = '\0';
        mainArea.textAt(0, line++, "Runes:%s", buffer);
    }
    if (c->saveGame->items & (ITEM_CANDLE | ITEM_BOOK | ITEM_BELL)) {
        buffer[0] = '\0';
        if (c->saveGame->items & ITEM_BELL) {
            strcat(buffer, getItemName(ITEM_BELL));
            strcat(buffer, " ");
        }
        if (c->saveGame->items & ITEM_BOOK) {
            strcat(buffer, getItemName(ITEM_BOOK));
            strcat(buffer, " ");
        }
        if (c->saveGame->items & ITEM_CANDLE) {
            strcat(buffer, getItemName(ITEM_CANDLE));
            buffer[15] = '\0';
        }
        mainArea.textAt(0, line++, "%s", buffer);
    }
    if (c->saveGame->items & (ITEM_KEY_C | ITEM_KEY_L | ITEM_KEY_T)) {
        j = 0;
        if (c->saveGame->items & ITEM_KEY_T)
            buffer[j++] = getItemName(ITEM_KEY_T)[0];
        if (c->saveGame->items & ITEM_KEY_L)
            buffer[j++] = getItemName(ITEM_KEY_L)[0];
        if (c->saveGame->items & ITEM_KEY_C)
            buffer[j++] = getItemName(ITEM_KEY_C)[0];
        buffer[j] = '\0';
        mainArea.textAt(0, line++, "3 Part Key:%s", buffer);
    }
    if (c->saveGame->items & ITEM_HORN)
        mainArea.textAt(0, line++, "%s", getItemName(ITEM_HORN));
    if (c->saveGame->items & ITEM_WHEEL)
        mainArea.textAt(0, line++, "%s", getItemName(ITEM_WHEEL));
    if (c->saveGame->items & ITEM_SKULL)
        mainArea.textAt(0, line++, "%s", getItemName(ITEM_SKULL));    
}

/**
 * Unmixed reagents in inventory.
 */
void StatsArea::showReagents(bool active)
{
    setTitle("Reagents");

    Menu::MenuItemList::iterator i;
    int line = 0,
        r = REAG_ASH;
    string shortcut ("A");

    reagentsMixMenu.show(&mainArea);

    for (i = reagentsMixMenu.begin(); i != reagentsMixMenu.end(); i++, r++)
    {
        if ((*i)->isVisible())
        {
            // Insert the reagent menu item shortcut character
            shortcut[0] = 'A'+r;
            if (active)
                mainArea.textAt(0, line++, "%s", mainArea.colorizeString(shortcut, FG_YELLOW, 0, 1).c_str());
            else
                mainArea.textAt(0, line++, "%s", shortcut.c_str());
        }
    }
}

/**
 * Mixed reagents in inventory.
 */
void StatsArea::showMixtures() {
    setTitle("Mixtures");

    int line = 0;
    int col = 0;
    for (int s = 0; s < SPELL_MAX; s++) {
        int n = c->saveGame->mixtures[s];
        if (n >= 100)
            n = 99;
        if (n >= 1) {
            mainArea.textAt(col, line++, "%c-%02d", s + 'A', n);
            if (line >= (STATS_AREA_HEIGHT)) {
                if (col >= 10)
                    break;
                line = 0;
                col += 5;
            }
        }
    }
}

void StatsArea::resetReagentsMenu() {
    Menu::MenuItemList::iterator current;
    int i = 0,
        row = 0;

    for (current = reagentsMixMenu.begin(); current != reagentsMixMenu.end(); current++)
    {
        if (c->saveGame->reagents[i++] > 0)
        {
            (*current)->setVisible(true);
            (*current)->setY(row++);
        }
        else (*current)->setVisible(false);
    }

    reagentsMixMenu.reset(false);
}

/**
 * Handles spell mixing for the Ultima V-style menu-system
 */
bool ReagentsMenuController::keyPressed(int key) {
    switch(key) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
        {
            /* select the corresponding reagent (if visible) */
            Menu::MenuItemList::iterator mi = menu->getById(key-'a');
            if ((*mi)->isVisible()) {
                menu->setCurrent(menu->getById(key-'a'));
                keyPressed(U4_SPACE);
            }
        } break;
    case U4_LEFT:
    case U4_RIGHT:
    case U4_SPACE:
        if (menu->isVisible()) {            
            MenuItem *item = *menu->getCurrent();
            
            /* change whether or not it's selected */
            item->setSelected(!item->isSelected());
                        
            if (item->isSelected())
                ingredients->addReagent((Reagent)item->getId());
            else
                ingredients->removeReagent((Reagent)item->getId());
        }
        break;
    case U4_ENTER:
        eventHandler->setControllerDone();
        break;

    case U4_ESC:
        ingredients->revert();
        eventHandler->setControllerDone();
        break;

    default:
        return MenuController::keyPressed(key);
    }

    return true;
}

