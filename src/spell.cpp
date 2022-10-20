/*
 * $Id: spell.cpp 3071 2014-07-26 18:01:08Z darren_janeczek $
 */
#include <string.h>

#include "u4.h"
#include "spell.h"
#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "direction.h"
#include "dungeon.h"
#include "error.h"
#include "event.h"
#include "game.h"
#include "location.h"
#include "map.h"
#include "mapmgr.h"
#include "music.h"
#include "creature.h"
#include "moongate.h"
#include "player.h"
#include "random.h"
#include "screen.h"
#include "settings.h"
#include "tile.h"
#include "tileset.h"
#include "utils.h"

SpellEffectCallback spellEffectCallback = NULL;

CombatController *spellCombatController();
void spellMagicAttack(const std::string &tilename, Direction dir, int minDamage, int maxDamage);
bool spellMagicAttackAt(const Coords &coords, MapTile attackTile, int attackDamage);

static int spellAwaken(int player);
static int spellBlink(int dir);
static int spellCure(int player);
static int spellDispel(int dir);
static int spellEField(int param);
static int spellFireball(int dir);
static int spellGate(int phase);
static int spellHeal(int player);
static int spellIceball(int dir);
static int spellJinx(int unused);
static int spellKill(int dir);
static int spellLight(int unused);
static int spellMMissle(int dir);
static int spellNegate(int unused);
static int spellOpen(int unused);
static int spellProtect(int unused);
static int spellRez(int player);
static int spellQuick(int unused);
static int spellSleep(int unused);
static int spellTremor(int unused);
static int spellUndead(int unused);
static int spellView(int unsued);
static int spellWinds(int fromdir);
static int spellXit(int unused);
static int spellYup(int unused);
static int spellZdown(int unused);

/* masks for reagents */
#define ASH (1 << REAG_ASH)
#define GINSENG (1 << REAG_GINSENG)
#define GARLIC (1 << REAG_GARLIC)
#define SILK (1 << REAG_SILK)
#define MOSS (1 << REAG_MOSS)
#define PEARL (1 << REAG_PEARL)
#define NIGHTSHADE (1 << REAG_NIGHTSHADE)
#define MANDRAKE (1 << REAG_MANDRAKE)

/* spell error messages */
static const struct {
    SpellCastError err;
    const char *msg;
} spellErrorMsgs[] = {
    { CASTERR_NOMIX, "None Mixed!\n" },
    { CASTERR_MPTOOLOW, "Not Enough MP!\n" },
    { CASTERR_FAILED, "Failed!\n" },
    { CASTERR_WRONGCONTEXT, "Not here!\n" },
    { CASTERR_COMBATONLY, "Combat only!\nFailed!\n" },
    { CASTERR_DUNGEONONLY, "Dungeon only!\nFailed!\n" },
    { CASTERR_WORLDMAPONLY, "Outdoors only!\nFailed!\n" }
};

static const Spell spells[] = {
    { "Awaken",       GINSENG | GARLIC,         CTX_ANY,        TRANSPORT_ANY,  &spellAwaken,  Spell::PARAM_PLAYER,  5 },
    { "Blink",        SILK | MOSS,              CTX_WORLDMAP,   TRANSPORT_FOOT_OR_HORSE,
                                                                                &spellBlink,   Spell::PARAM_DIR,     15 },
    { "Cure",         GINSENG | GARLIC,         CTX_ANY,        TRANSPORT_ANY,  &spellCure,    Spell::PARAM_PLAYER,  5 },
    { "Dispell",      ASH | GARLIC | PEARL,     CTX_ANY,        TRANSPORT_ANY,  &spellDispel,  Spell::PARAM_DIR,     20 },
    { "Energy Field", ASH | SILK | PEARL,       (LocationContext)(CTX_COMBAT | CTX_DUNGEON),
                                                                TRANSPORT_ANY,  &spellEField,  Spell::PARAM_TYPEDIR, 10 },
    { "Fireball",     ASH | PEARL,              CTX_COMBAT,     TRANSPORT_ANY,  &spellFireball,Spell::PARAM_DIR,     15 },
    { "Gate",         ASH | PEARL | MANDRAKE,   CTX_WORLDMAP,   TRANSPORT_FOOT_OR_HORSE,
                                                                                &spellGate,    Spell::PARAM_PHASE,   40 },
    { "Heal",         GINSENG | SILK,           CTX_ANY,        TRANSPORT_ANY,  &spellHeal,    Spell::PARAM_PLAYER,  10 },
    { "Iceball",      PEARL | MANDRAKE,         CTX_COMBAT,     TRANSPORT_ANY,  &spellIceball, Spell::PARAM_DIR,     20 },
    { "Jinx",         PEARL | NIGHTSHADE | MANDRAKE,
                                                CTX_ANY,        TRANSPORT_ANY,  &spellJinx,    Spell::PARAM_NONE,    30 },
    { "Kill",         PEARL | NIGHTSHADE,       CTX_COMBAT,     TRANSPORT_ANY,  &spellKill,    Spell::PARAM_DIR,     25 },
    { "Light",        ASH,                      CTX_DUNGEON,    TRANSPORT_ANY,  &spellLight,   Spell::PARAM_NONE,    5 },
    { "Magic missile", ASH | PEARL,             CTX_COMBAT,     TRANSPORT_ANY,  &spellMMissle, Spell::PARAM_DIR,     5 },
    { "Negate",       ASH | GARLIC | MANDRAKE,  CTX_ANY,        TRANSPORT_ANY,  &spellNegate,  Spell::PARAM_NONE,    20 },
    { "Open",         ASH | MOSS,               CTX_ANY,        TRANSPORT_ANY,  &spellOpen,    Spell::PARAM_NONE,    5 },
    { "Protection",   ASH | GINSENG | GARLIC,   CTX_ANY,        TRANSPORT_ANY,  &spellProtect, Spell::PARAM_NONE,    15 },
    { "Quickness",    ASH | GINSENG | MOSS,     CTX_ANY,        TRANSPORT_ANY,  &spellQuick,   Spell::PARAM_NONE,    20 },
    { "Resurrect",    ASH | GINSENG | GARLIC | SILK | MOSS | MANDRAKE,
                                                CTX_NON_COMBAT, TRANSPORT_ANY,  &spellRez,     Spell::PARAM_PLAYER,  45 },
    { "Sleep",        SILK | GINSENG,           CTX_COMBAT,     TRANSPORT_ANY,  &spellSleep,   Spell::PARAM_NONE,    15 },
    { "Tremor",       ASH | MOSS | MANDRAKE,    CTX_COMBAT,     TRANSPORT_ANY,  &spellTremor,  Spell::PARAM_NONE,    30 },
    { "Undead",       ASH | GARLIC,             CTX_COMBAT,     TRANSPORT_ANY,  &spellUndead,  Spell::PARAM_NONE,    15 },
    { "View",         NIGHTSHADE | MANDRAKE,    CTX_NON_COMBAT, TRANSPORT_ANY,  &spellView,    Spell::PARAM_NONE,    15 },
    { "Winds",        ASH | MOSS,               CTX_WORLDMAP,   TRANSPORT_ANY,  &spellWinds,   Spell::PARAM_FROMDIR, 10 },
    { "X-it",         ASH | SILK | MOSS,        CTX_DUNGEON,    TRANSPORT_ANY,  &spellXit,     Spell::PARAM_NONE,    15 },
    { "Y-up",         SILK | MOSS,              CTX_DUNGEON,    TRANSPORT_ANY,  &spellYup,     Spell::PARAM_NONE,    10 },
    { "Z-down",       SILK | MOSS,              CTX_DUNGEON,    TRANSPORT_ANY,  &spellZdown,   Spell::PARAM_NONE,    5 }
};

#define N_SPELLS (sizeof(spells) / sizeof(spells[0]))

void spellSetEffectCallback(SpellEffectCallback callback) {
    spellEffectCallback = callback;
}

Ingredients::Ingredients() {
    memset(reagents, 0, sizeof(reagents));
}

bool Ingredients::addReagent(Reagent reagent) {
    zu4_assert(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    if (c->party->getReagent(reagent) < 1)
        return false;
    c->party->adjustReagent(reagent, -1);
    reagents[reagent]++;
    return true;
}

bool Ingredients::removeReagent(Reagent reagent) {
    zu4_assert(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    if (reagents[reagent] == 0)
        return false;
    c->party->adjustReagent(reagent, 1);
    reagents[reagent]--;
    return true;
}

int Ingredients::getReagent(Reagent reagent) const {
    zu4_assert(reagent < REAG_MAX, "invalid reagent: %d", reagent);
    return reagents[reagent];
}

void Ingredients::revert() {
    int reg;

    for (reg = 0; reg < REAG_MAX; reg++) {
        c->saveGame->reagents[reg] += reagents[reg];
        reagents[reg] = 0;
    }
}

bool Ingredients::checkMultiple(int batches) const {
    for (int i = 0; i < REAG_MAX; i++) {
        /* see if there's enough reagents to mix (-1 because one is already counted) */
        if (reagents[i] > 0 && c->saveGame->reagents[i] < batches - 1) {
            return false;
        }
    }
    return true;
}

void Ingredients::multiply(int batches) {
    zu4_assert(checkMultiple(batches), "not enough reagents to multiply ingredients by %d\n", batches);
    for (int i = 0; i < REAG_MAX; i++) {
        if (reagents[i] > 0) {
            c->saveGame->reagents[i] -= batches - 1;
            reagents[i] += batches - 1;
        }
    }
}

const char *spellGetName(unsigned int spell) {
    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].name;
}

int spellGetRequiredMP(unsigned int spell) {
    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].mp;
}

LocationContext spellGetContext(unsigned int spell) {
    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].context;
}

TransportContext spellGetTransportContext(unsigned int spell) {
    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].transportContext;
}

std::string spellGetErrorMessage(unsigned int spell, SpellCastError error) {
    unsigned int i;
    SpellCastError err = error;

    /* try to find a more specific error message */
    if (err == CASTERR_WRONGCONTEXT) {
        switch(spells[spell].context) {
            case CTX_COMBAT: err = CASTERR_COMBATONLY; break;
            case CTX_DUNGEON: err = CASTERR_DUNGEONONLY; break;
            case CTX_WORLDMAP: err = CASTERR_WORLDMAPONLY; break;
            default: break;
        }
    }

    /* find the message that we're looking for and return it! */
    for (i = 0; i < sizeof(spellErrorMsgs) / sizeof(spellErrorMsgs[0]); i++) {
        if (err == spellErrorMsgs[i].err)
            return std::string(spellErrorMsgs[i].msg);
    }

    return std::string();
}

/**
 * Mix reagents for a spell.  Fails and returns false if the reagents
 * selected were not correct.
 */
int spellMix(unsigned int spell, const Ingredients *ingredients) {
    int regmask, reg;

    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);

    regmask = 0;
    for (reg = 0; reg < REAG_MAX; reg++) {
        if (ingredients->getReagent((Reagent) reg) > 0)
            regmask |= (1 << reg);
    }

    if (regmask != spells[spell].components)
        return 0;

    c->saveGame->mixtures[spell]++;

    return 1;
}

Spell::Param spellGetParamType(unsigned int spell) {
    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);

    return spells[spell].paramType;
}

/**
 * Checks some basic prerequistes for casting a spell.  Returns an
 * error if no mixture is available, the context is invalid, or the
 * character doesn't have enough magic points.
 */
SpellCastError spellCheckPrerequisites(unsigned int spell, int character) {
    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);
    zu4_assert(character >= 0 && character < c->saveGame->members, "character out of range: %d", character);

    if (c->saveGame->mixtures[spell] == 0)
        return CASTERR_NOMIX;

    if ((c->location->context & spells[spell].context) == 0)
        return CASTERR_WRONGCONTEXT;

    if ((c->transportContext & spells[spell].transportContext) == 0)
        return CASTERR_FAILED;

    if (c->party->member(character)->getMp() < spells[spell].mp)
        return CASTERR_MPTOOLOW;

    return CASTERR_NOERROR;
}

/**
 * Casts spell.  Fails and returns false if the spell cannot be cast.
 * The error code is updated with the reason for failure.
 */
bool spellCast(unsigned int spell, int character, int param, SpellCastError *error, bool spellEffect) {
    int subject = (spells[spell].paramType == Spell::PARAM_PLAYER) ? param : -1;
    PartyMember *p = c->party->member(character);

    zu4_assert(spell < N_SPELLS, "invalid spell: %d", spell);
    zu4_assert(character >= 0 && character < c->saveGame->members, "character out of range: %d", character);

    *error = spellCheckPrerequisites(spell, character);

    // subtract the mixture for even trying to cast the spell
    AdjustValueMin(c->saveGame->mixtures[spell], -1, 0);

    if (*error != CASTERR_NOERROR)
        return false;

    // If there's a negate magic aura, spells fail!
    if (c->aura->type == AURA_NEGATE) {
        *error = CASTERR_FAILED;
        return false;
    }

    // subtract the mp needed for the spell
    p->adjustMp(-spells[spell].mp);

    if (spellEffect) {
		int time;
		/* recalculate spell speed - based on 5/sec */
		float MP_OF_LARGEST_SPELL = 45;
		int spellMp = spells[spell].mp;
		time = int(10000.0 / settings.spellEffectSpeed  *  spellMp / MP_OF_LARGEST_SPELL);
		zu4_snd_play(SOUND_PREMAGIC_MANA_JUMBLE, false, time);
		EventHandler::wait_msecs(time);

        (*spellEffectCallback)(spell + 'a', subject, SOUND_MAGIC);
    }

    if (!(*spells[spell].spellFunc)(param)) {
        *error = CASTERR_FAILED;
        return false;
    }

    return true;
}

CombatController *spellCombatController() {
    CombatController *cc = dynamic_cast<CombatController *>(eventHandler->getController());
    return cc;
}

/**
 * Makes a special magic ranged attack in the given direction
 */
void spellMagicAttack(const std::string &tilename, Direction dir, int minDamage, int maxDamage) {
    CombatController *controller = spellCombatController();
    PartyMemberVector *party = controller->getParty();

    MapTile tile = c->location->map->tileset->getByName(tilename)->getId();

    int attackDamage = ((minDamage >= 0) && (minDamage < maxDamage)) ?
        zu4_random((maxDamage + 1) - minDamage) + minDamage :
        maxDamage;

    std::vector<Coords> path = gameGetDirectionalActionPath(MASK_DIR(dir), MASK_DIR_ALL, (*party)[controller->getFocus()]->getCoords(),
                                                       1, 11, Tile::canAttackOverTile, false);
    for (std::vector<Coords>::iterator i = path.begin(); i != path.end(); i++) {
        if (spellMagicAttackAt(*i, tile, attackDamage))
            return;
    }
}

bool spellMagicAttackAt(const Coords &coords, MapTile attackTile, int attackDamage) {
    bool objectHit = false;
//    int attackdelay = MAX_BATTLE_SPEED - settings.battleSpeed;
    CombatMap *cm = getCombatMap();

    Creature *creature = cm->creatureAt(coords);

    if (!creature) {
    	GameController::flashTile(coords, attackTile,2);
    }
    else {
        objectHit = true;

        /* show the 'hit' tile */
        zu4_snd_play(SOUND_NPC_STRUCK, true, -1);
        GameController::flashTile(coords, attackTile, 3);

        /* apply the damage to the creature */
        CombatController *controller = spellCombatController();
        controller->getCurrentPlayer()->dealDamage(creature, attackDamage);
        GameController::flashTile(coords, attackTile, 1);
    }

    return objectHit;
}

static int spellAwaken(int player) {
    zu4_assert(player < 8, "player out of range: %d", player);
    PartyMember *p = c->party->member(player);

    if ((player < c->party->size()) && (p->getStatus() == STAT_SLEEPING)) {
        p->wakeUp();
        return 1;
    }

    return 0;
}

static int spellBlink(int dir) {
    int i,
        failed = 0,
        distance,
        diff,
        *var;
    Direction reverseDir = dirReverse((Direction)dir);
    Coords coords = c->location->coords;

    /* Blink doesn't work near the mouth of the abyss */
    /* Note: This means you can teleport to Hythloth from the top of the map,
       and that you can teleport to the abyss from the left edge of the map,
       Unfortunately, this matches the bugs in the game. :(  Consider fixing. */
    if (coords.x >= 192 && coords.y >= 192)
        return 0;

    /* figure out what numbers we're working with */
    var = (dir & (DIR_WEST | DIR_EAST)) ? &coords.x : &coords.y;

    /* find the distance we are going to move */
    distance = (*var) % 0x10;
    if (dir == DIR_EAST || dir == DIR_SOUTH)
        distance = 0x10 - distance;

    /* see if we move another 16 spaces over */
    diff = 0x10 - distance;
    if ((diff > 0) && (zu4_random(diff * diff) > distance))
        distance += 0x10;

    /* test our distance, and see if it works */
    for (i = 0; i < distance; i++)
        movedir(&coords, (Direction)dir, c->location->map);

    i = distance;
    /* begin walking backward until you find a valid spot */
    while ((i-- > 0) && !c->location->map->tileTypeAt(coords, WITH_OBJECTS)->isWalkable())
        movedir(&coords, reverseDir, c->location->map);

    if (c->location->map->tileTypeAt(coords, WITH_OBJECTS)->isWalkable()) {
        /* we didn't move! */
        if (zu4_coords_equal(c->location->coords, coords))
            failed = 1;

        //c->location->coords = coords;
        c->location->coords.x = coords.x;
        c->location->coords.y = coords.y;
        c->location->coords.z = coords.z;

    } else failed = 1;

    return (failed ? 0 : 1);
}

static int spellCure(int player) {
    zu4_assert(player < 8, "player out of range: %d", player);

    GameController::flashTile(c->party->member(player)->getCoords(), "wisp", 1);
    return c->party->member(player)->heal(HT_CURE);
}

static int spellDispel(int dir) {
    MapTile *tile;
    Coords field;

    /*
     * get the location of the avatar (or current party member, if in battle)
     */
    c->location->getCurrentPosition(&field);

    /*
     * find where we want to dispel the field
     */
    movedir(&field, (Direction)dir, c->location->map);

    GameController::flashTile(field, "wisp", 2);
    /*
     * if there is a field annotation, remove it and replace it with a valid
     * replacement annotation.  We do this because sometimes dungeon triggers
     * create annotations, that, if just removed, leave a wall tile behind
     * (or other unwalkable surface).  So, we need to provide a valid replacement
     * annotation to fill in the gap :)
     */
    Annotation::List a = c->location->map->annotations->allAt(field);
    if (a.size() > 0) {
        Annotation::List::iterator i;
        for (i = a.begin(); i != a.end(); i++) {
            if (i->getTile().getTileType()->canDispel()) {

                /*
                 * get a replacement tile for the field
                 */
                MapTile newTile(c->location->getReplacementTile(field, i->getTile().getTileType()));

                c->location->map->annotations->remove(*i);
                c->location->map->annotations->add(field, newTile, false, true);
                return 1;
            }
        }
    }

    /*
     * if the map tile itself is a field, overlay it with a replacement tile
     */

    tile = c->location->map->tileAt(field, WITHOUT_OBJECTS);
    if (!tile->getTileType()->canDispel())
        return 0;

    /*
     * get a replacement tile for the field
     */
    MapTile newTile(c->location->getReplacementTile(field, tile->getTileType()));

    c->location->map->annotations->add(field, newTile, false, true);

    return 1;
}

static int spellEField(int param) {
    MapTile fieldTile(0);
    int fieldType;
    int dir;
    Coords coords;

    /* Unpack fieldType and direction */
    fieldType = param >> 4;
    dir = param & 0xF;

    /* Make sure params valid */
    switch (fieldType) {
        case ENERGYFIELD_FIRE: fieldTile = c->location->map->tileset->getByName("fire_field")->getId(); break;
        case ENERGYFIELD_LIGHTNING: fieldTile = c->location->map->tileset->getByName("energy_field")->getId(); break;
        case ENERGYFIELD_POISON: fieldTile = c->location->map->tileset->getByName("poison_field")->getId(); break;
        case ENERGYFIELD_SLEEP: fieldTile = c->location->map->tileset->getByName("sleep_field")->getId(); break;
        default: return 0; break;
    }

    Coords mc(coords);
    c->location->getCurrentPosition(&mc);
    coords.x = mc.x; coords.y = mc.y; coords.z = mc.z;

    movedir(&coords, (Direction)dir, c->location->map);
    if (MAP_IS_OOB(c->location->map, coords))
        return 0;
    else {
        /*
         * Observed behaviour on Amiga version of Ultima IV:
         * Field cast on other field: Works, unless original field is lightning
         * in which case it doesn't.
         * Field cast on creature: Works, creature remains the visible tile
         * Field cast on top of field and then dispel = no fields left
         * The code below seems to produce this behaviour.
         */
        const Tile *tile = c->location->map->tileTypeAt(coords, WITH_GROUND_OBJECTS);
        if (!tile->isWalkable()) return 0;

        /* Get rid of old field, if any */
        Annotation::List a = c->location->map->annotations->allAt(coords);
        if (a.size() > 0) {
            Annotation::List::iterator i;
            for (i = a.begin(); i != a.end(); i++) {
                if (i->getTile().getTileType()->canDispel())
                    c->location->map->annotations->remove(*i);
            }
        }

        c->location->map->annotations->add(coords, fieldTile);
    }

    return 1;
}

static int spellFireball(int dir) {
    spellMagicAttack("hit_flash", (Direction)dir, 24, 128);
    return 1;
}

static int spellGate(int phase) {
    const Coords *moongate;

    GameController::flashTile(c->location->coords, "moongate", 2);

    moongate = moongateGetGateCoordsForPhase(phase);
    if (moongate)
        c->location->coords = *moongate;

    return 1;
}

static int spellHeal(int player) {
    zu4_assert(player < 8, "player out of range: %d", player);

    GameController::flashTile(c->party->member(player)->getCoords(), "wisp", 1);
    c->party->member(player)->heal(HT_HEAL);
    return 1;
}

static int spellIceball(int dir) {
    spellMagicAttack("magic_flash", (Direction)dir, 32, 224);
    return 1;
}

static int spellJinx(int unused) {
    c->aura->set(AURA_JINX, 10);
    return 1;
}

static int spellKill(int dir) {
    spellMagicAttack("whirlpool", (Direction)dir, -1, 232);
    return 1;
}

static int spellLight(int unused) {
    c->party->lightTorch(100, false);
    return 1;
}

static int spellMMissle(int dir) {
    spellMagicAttack("miss_flash", (Direction)dir, 64, 16);
    return 1;
}

static int spellNegate(int unused) {
    c->aura->set(AURA_NEGATE, 10);
    return 1;
}

static int spellOpen(int unused) {
    getChest(-2);   // HACK: -2 will not prompt for opener
    return 1;
}

static int spellProtect(int unused) {
    c->aura->set(AURA_PROTECTION, 10);
    return 1;
}

static int spellRez(int player) {
    zu4_assert(player < 8, "player out of range: %d", player);

    return c->party->member(player)->heal(HT_RESURRECT);
}

static int spellQuick(int unused) {
    c->aura->set(AURA_QUICKNESS, 10);
    return 1;
}

static int spellSleep(int unused) {
    CombatMap *cm = getCombatMap();
    CreatureVector creatures = cm->getCreatures();
    CreatureVector::iterator i;

    /* try to put each creature to sleep */

    for (i = creatures.begin(); i != creatures.end(); i++) {
        Creature *m = *i;
        Coords coords = m->getCoords();
        GameController::flashTile(coords, "wisp", 1);
        if ((m->getResists() != EFFECT_SLEEP) &&
            zu4_random(0xFF) >= m->getHp())
        {
        	zu4_snd_play(SOUND_POISON_EFFECT, true, -1);
            m->putToSleep();
            GameController::flashTile(coords, "sleep_field", 3);
        }
        else
        	zu4_snd_play(SOUND_EVADE, true, -1);
    }

    return 1;
}

static int spellTremor(int unused) {
    CombatController *ct = spellCombatController();
    CreatureVector creatures = ct->getMap()->getCreatures();
    CreatureVector::iterator i;

    for (i = creatures.begin(); i != creatures.end(); i++) {
        Creature *m = *i;

        Coords coords = m->getCoords();
        //GameController::flashTile(coords, "rocks", 1);

        /* creatures with over 192 hp are unaffected */
        if (m->getHp() > 192) {
            zu4_snd_play(SOUND_EVADE, true, -1);
            continue;
        }
        else {
            /* Deal maximum damage to creature */
            if (zu4_random(2) == 0) {
                zu4_snd_play(SOUND_NPC_STRUCK, true, -1);
                GameController::flashTile(coords, "hit_flash", 3);
                ct->getCurrentPlayer()->dealDamage(m, 0xFF);
            }
            /* Deal enough damage to creature to make it flee */
            else if (zu4_random(2) == 0) {
                zu4_snd_play(SOUND_NPC_STRUCK, true, -1);
                GameController::flashTile(coords, "hit_flash", 2);
                if (m->getHp() > 23)
                    ct->getCurrentPlayer()->dealDamage(m, m->getHp()-23);
            }
            else
            {
                zu4_snd_play(SOUND_EVADE, true, -1);
            }
        }
    }

    return 1;
}

static int spellUndead(int unused) {
    CombatController *ct = spellCombatController();
    CreatureVector creatures = ct->getMap()->getCreatures();
    CreatureVector::iterator i;

    for (i = creatures.begin(); i != creatures.end(); i++) {
        Creature *m = *i;
        if (m && m->isUndead() && zu4_random(2) == 0)
            m->setHp(23);
    }

    return 1;
}

static int spellView(int unsued) {
    peer(false);
    return 1;
}

static int spellWinds(int fromdir) {
    c->windDirection = fromdir;
    return 1;
}

static int spellXit(int unused) {
    if (!c->location->map->isWorldMap()) {
        screenMessage("Leaving...\n");
        game->exitToParentMap();
        zu4_music_play(c->location->map->music);
        return 1;
    }
    return 0;
}

static int spellYup(int unused) {
    Coords coords = c->location->coords;
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);

    /* can't cast in the Abyss */
    if (c->location->map->id == MAP_ABYSS)
        return 0;
    /* staying in the dungeon */
    else if (coords.z > 0) {
        for (int i = 0; i < 0x20; i++) {
            coords = (Coords){zu4_random(8), zu4_random(8), c->location->coords.z - 1};
            if (dungeon->validTeleportLocation(coords)) {
                c->location->coords = coords;
                return 1;
            }
        }
    /* exiting the dungeon */
    } else {
        screenMessage("Leaving...\n");
        game->exitToParentMap();
        zu4_music_play(c->location->map->music);
        return 1;
    }

    /* didn't find a place to go, failed! */
    return 0;
}

static int spellZdown(int unused) {
    Coords coords = c->location->coords;
    Dungeon *dungeon = dynamic_cast<Dungeon *>(c->location->map);

    /* can't cast in the Abyss */
    if (c->location->map->id == MAP_ABYSS)
        return 0;
    /* can't go lower than level 8 */
    else if (coords.z >= 7)
        return 0;
    else {
        for (int i = 0; i < 0x20; i++) {
            coords = (Coords){zu4_random(8), zu4_random(8), c->location->coords.z + 1};
            if (dungeon->validTeleportLocation(coords)) {
                c->location->coords = coords;
                return 1;
            }
        }
    }

    /* didn't find a place to go, failed! */
    return 0;
}

const Spell* getSpell(int i) {return &spells[i];}
