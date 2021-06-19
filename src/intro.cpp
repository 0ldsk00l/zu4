/*
 * $Id: intro.cpp 3078 2014-07-30 00:48:35Z darren_janeczek $
 */

#include <SDL.h>

#include <algorithm>
#include <cstring>
#include "u4.h"

#include "intro.h"

#include "error.h"
#include "event.h"
#include "imagemgr.h"
#include "menu.h"
#include "music.h"
#include "sound.h"
#include "player.h"
#include "random.h"
#include "savegame.h"
#include "screen.h"
#include "settings.h"
#include "shrine.h"
#include "tileset.h"
#include "tilemap.h"
#include "u4file.h"
#include "utils.h"

using namespace std;

static bool notblanked = true;

extern bool useProfile;
extern string profileName;
extern bool quit;

IntroController *intro = NULL;

#define INTRO_MAP_HEIGHT 5
#define INTRO_MAP_WIDTH 19
#define INTRO_TEXT_X 0
#define INTRO_TEXT_Y 19
#define INTRO_TEXT_WIDTH 40
#define INTRO_TEXT_HEIGHT 6

#define GYP_PLACES_FIRST 0
#define GYP_PLACES_TWOMORE 1
#define GYP_PLACES_LAST 2
#define GYP_UPON_TABLE 3
#define GYP_SEGUE1 13
#define GYP_SEGUE2 14

class IntroObjectState {
public:
    IntroObjectState() : x(0), y(0), tile(0) {}
    int x, y;
    MapTile tile; /* base tile + tile frame */    
};

/* temporary place-holder for settings changes */
SettingsData settingsChanged;

const int IntroBinData::INTRO_TEXT_OFFSET = 17445 - 1;  // (start at zero)
const int IntroBinData::INTRO_MAP_OFFSET = 30339;
const int IntroBinData::INTRO_FIXUPDATA_OFFSET = 29806;
const int IntroBinData::INTRO_SCRIPT_TABLE_SIZE = 548;
const int IntroBinData::INTRO_SCRIPT_TABLE_OFFSET = 30434;
const int IntroBinData::INTRO_BASETILE_TABLE_SIZE = 15;
const int IntroBinData::INTRO_BASETILE_TABLE_OFFSET = 16584;
const int IntroBinData::BEASTIE1_FRAMES = 0x80;
const int IntroBinData::BEASTIE2_FRAMES = 0x40;
const int IntroBinData::BEASTIE_FRAME_TABLE_OFFSET = 0x7380;
const int IntroBinData::BEASTIE1_FRAMES_OFFSET = 0;
const int IntroBinData::BEASTIE2_FRAMES_OFFSET = 0x78;

static char *introQuestions[28];
static char *introText[24];
static char *introGypsy[15];

IntroBinData::IntroBinData() : 
    sigData(NULL), 
    scriptTable(NULL), 
    baseTileTable(NULL),
    beastie1FrameTable(NULL),
    beastie2FrameTable(NULL) {
}

IntroBinData::~IntroBinData() {
    if (sigData)
        delete [] sigData;
    if (scriptTable)
        delete [] scriptTable;
    if (baseTileTable)
        delete [] baseTileTable;
    if (beastie1FrameTable)
        delete [] beastie1FrameTable;
    if (beastie2FrameTable)
        delete [] beastie2FrameTable;

    for (int i = 0; i < 28; i++) { free(introQuestions[i]); }
    for (int i = 0; i < 24; i++) { free(introText[i]); }
    for (int i = 0; i < 15; i++) { free(introGypsy[i]); }
}

bool IntroBinData::load() {
    int i;

    U4FILE *title = u4fopen("title.exe");
    if (!title)
        return false;

    zu4_read_strtable(title, INTRO_TEXT_OFFSET, (char**)introQuestions, 28);
    zu4_read_strtable(title, -1, (char**)introText, 24);
    zu4_read_strtable(title, -1, (char**)introGypsy, 15);

    if (sigData)
        delete sigData;
    sigData = new unsigned char[533];
    u4fseek(title, INTRO_FIXUPDATA_OFFSET, SEEK_SET);
    u4fread(title, sigData, 1, 533);

    u4fseek(title, INTRO_MAP_OFFSET, SEEK_SET);
    introMap.resize(INTRO_MAP_WIDTH * INTRO_MAP_HEIGHT, MapTile(0));
    for (i = 0; i < INTRO_MAP_HEIGHT * INTRO_MAP_WIDTH; i++)        
        introMap[i] = TileMap::get("base")->translate(u4fgetc(title));
        
    u4fseek(title, INTRO_SCRIPT_TABLE_OFFSET, SEEK_SET);
    scriptTable = new unsigned char[INTRO_SCRIPT_TABLE_SIZE];
    for (i = 0; i < INTRO_SCRIPT_TABLE_SIZE; i++)
        scriptTable[i] = u4fgetc(title);

    u4fseek(title, INTRO_BASETILE_TABLE_OFFSET, SEEK_SET);
    baseTileTable = new Tile*[INTRO_BASETILE_TABLE_SIZE];
    for (i = 0; i < INTRO_BASETILE_TABLE_SIZE; i++) {
        MapTile tile = TileMap::get("base")->translate(u4fgetc(title));
        baseTileTable[i] = Tileset::get("base")->get(tile.id);
    }

    /* --------------------------
       load beastie frame table 1
       -------------------------- */
    beastie1FrameTable = new unsigned char[BEASTIE1_FRAMES];
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE1_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE1_FRAMES; i++) {
        beastie1FrameTable[i] = u4fgetc(title);
    }

    /* --------------------------
       load beastie frame table 2
       -------------------------- */
    beastie2FrameTable = new unsigned char[BEASTIE2_FRAMES];
    u4fseek(title, BEASTIE_FRAME_TABLE_OFFSET + BEASTIE2_FRAMES_OFFSET, SEEK_SET);
    for (i = 0; i < BEASTIE2_FRAMES; i++) {
        beastie2FrameTable[i] = u4fgetc(title);
    }

    u4fclose(title);

    return true;
}

IntroController::IntroController() : 
    Controller(1), 
    backgroundArea(),
    menuArea(1 * CHAR_WIDTH, 13 * CHAR_HEIGHT, 38, 11),
    extendedMenuArea(2 * CHAR_WIDTH, 10 * CHAR_HEIGHT, 36, 13),
    questionArea(INTRO_TEXT_X * CHAR_WIDTH, INTRO_TEXT_Y * CHAR_HEIGHT, INTRO_TEXT_WIDTH, INTRO_TEXT_HEIGHT),
    mapArea(BORDER_WIDTH, (TILE_HEIGHT * 6) + BORDER_HEIGHT, INTRO_MAP_WIDTH, INTRO_MAP_HEIGHT, "base"),
    binData(NULL),
    titles(),                   // element list
    title(titles.begin()),      // element iterator
    transparentIndex(13),       // palette index for transparency
    transparentColor(),         // palette color for transparency
    bSkipTitles(false)
{
    // initialize menus
    confMenu.setTitle("zu4 Configuration:", 0, 0);
    confMenu.add(MI_CONF_VIDEO,               "\010 Video Options",              2,  2,/*'v'*/  2);
    confMenu.add(MI_CONF_SOUND,               "\010 Sound Options",              2,  3,/*'s'*/  2);
    confMenu.add(MI_CONF_INPUT,               "\010 Input Options",              2,  4,/*'i'*/  2);
    confMenu.add(MI_CONF_SPEED,               "\010 Speed Options",              2,  5,/*'p'*/  3);
    confMenu.add(MI_CONF_01, new BoolMenuItem("Game Enhancements         %s",    2,  7,/*'e'*/  5, &settingsChanged.enhancements));
    confMenu.add(MI_CONF_GAMEPLAY,            "\010 Enhanced Gameplay Options",  2,  9,/*'g'*/ 11);
    confMenu.add(MI_CONF_INTERFACE,           "\010 Enhanced Interface Options", 2, 10,/*'n'*/ 12);
    confMenu.add(CANCEL,                      "\017 Main Menu",                  2, 12,/*'m'*/  2);
    confMenu.addShortcutKey(CANCEL, ' ');
    confMenu.setClosesMenu(CANCEL);

    /* set the default visibility of the two enhancement menus */
    confMenu.getItemById(MI_CONF_GAMEPLAY)->setVisible(settings.enhancements);
    confMenu.getItemById(MI_CONF_INTERFACE)->setVisible(settings.enhancements);

    videoMenu.setTitle("Video Options:", 0, 0);
    videoMenu.add(MI_VIDEO_CONF_GFX, 						 "\010 Game Graphics Options",  2,  2,/*'g'*/  2);
    videoMenu.add(MI_VIDEO_04,    		new IntMenuItem		("Scale                x%d", 2,  4,/*'s'*/  0, reinterpret_cast<int *>(&settingsChanged.scale), 1, 5, 1));
    videoMenu.add(MI_VIDEO_05,  (		new BoolMenuItem	("Mode                 %s",  2,  5,/*'m'*/  0, &settingsChanged.fullscreen))->setValueStrings("Fullscreen", "Window"));
    videoMenu.add(MI_VIDEO_06,    		new IntMenuItem		("Gamma                %s",  2,  6,/*'a'*/  1, &settingsChanged.gamma, 50, 150, 10, MENU_OUTPUT_GAMMA));
    videoMenu.add(USE_SETTINGS,                   "\010 Use These Settings",  2, 11,/*'u'*/  2);
    videoMenu.add(CANCEL,                         "\010 Cancel",              2, 12,/*'c'*/  2);
    videoMenu.addShortcutKey(CANCEL, ' ');
    videoMenu.setClosesMenu(USE_SETTINGS);
    videoMenu.setClosesMenu(CANCEL);
    
    gfxMenu.setTitle("Game Graphics Options", 0,0);
    //IntMenuItem(string text, short x, short y, int shortcutKey, int *val, int min, int max, int increment, menuOutputType output=MENU_OUTPUT_INT);
    gfxMenu.add(MI_GFX_SCHEME, new IntMenuItem							("Graphics Scheme    %d", 2, 2, /*'G'*/ 0, &settingsChanged.videoType, 0, 1, 1));
    gfxMenu.add(MI_VIDEO_02, 				new IntMenuItem				("Gem Layout         %d",  2,  4,/*'e'*/  1, &settingsChanged.gemLayout, 0, 1, 1));
    gfxMenu.add(MI_VIDEO_03, 			new IntMenuItem					("Line Of Sight      %d",  2,  5,/*'l'*/  0, &settingsChanged.lineOfSight, 0, 1, 1));
    gfxMenu.add(MI_VIDEO_07,   		new BoolMenuItem					("Screen Shaking     %s",  2, 6,/*'k'*/ 8, &settingsChanged.screenShakes));
    gfxMenu.add(MI_GFX_RETURN,               "\010 Return to Video Options",              2,  12,/*'r'*/  2);
    gfxMenu.setClosesMenu(MI_GFX_RETURN);


    soundMenu.setTitle("Sound Options:", 0, 0);
    soundMenu.add(MI_SOUND_01,  new IntMenuItem("Music Volume         %s", 2,  2,/*'m'*/  0, &settingsChanged.musicVol, 0, MAX_VOLUME, 1, MENU_OUTPUT_VOLUME));
    soundMenu.add(MI_SOUND_02,  new IntMenuItem("Sound Effect Volume  %s", 2,  3,/*'s'*/  0, &settingsChanged.soundVol, 0, MAX_VOLUME, 1, MENU_OUTPUT_VOLUME));
    soundMenu.add(MI_SOUND_03, new BoolMenuItem("Fading               %s", 2,  4,/*'f'*/  0, &settingsChanged.volumeFades));
    soundMenu.add(USE_SETTINGS,                 "\010 Use These Settings", 2, 11,/*'u'*/  2);
    soundMenu.add(CANCEL,                       "\010 Cancel",             2, 12,/*'c'*/  2);
    soundMenu.addShortcutKey(CANCEL, ' ');
    soundMenu.setClosesMenu(USE_SETTINGS);
    soundMenu.setClosesMenu(CANCEL);

    inputMenu.setTitle("Keyboard Options:", 0, 0);
    inputMenu.add(MI_INPUT_01,  new IntMenuItem("Repeat Delay        %4d msec", 2,  2,/*'d'*/  7, &settingsChanged.keydelay, 100, MAX_KEY_DELAY, 100));
    inputMenu.add(MI_INPUT_02,  new IntMenuItem("Repeat Interval     %4d msec", 2,  3,/*'i'*/  7, &settingsChanged.keyinterval, 10, MAX_KEY_INTERVAL, 10));
    inputMenu.add(USE_SETTINGS,                 "\010 Use These Settings",      2, 11,/*'u'*/  2);
    inputMenu.add(CANCEL,                       "\010 Cancel",                  2, 12,/*'c'*/  2);
    inputMenu.addShortcutKey(CANCEL, ' ');
    inputMenu.setClosesMenu(USE_SETTINGS);
    inputMenu.setClosesMenu(CANCEL);
    
    speedMenu.setTitle("Speed Options:", 0, 0);
    speedMenu.add(MI_SPEED_01, new IntMenuItem("Game Cycles per Second    %3d",      2,  2,/*'g'*/  0, &settingsChanged.gameCyclesPerSecond, 1, MAX_CYCLES_PER_SECOND, 1));
    speedMenu.add(MI_SPEED_02, new IntMenuItem("Battle Speed              %3d",      2,  3,/*'b'*/  0, &settingsChanged.battleSpeed, 1, MAX_BATTLE_SPEED, 1));
    speedMenu.add(MI_SPEED_03, new IntMenuItem("Spell Effect Length       %s",       2,  4,/*'p'*/  1, &settingsChanged.spellEffectSpeed, 1, MAX_SPELL_EFFECT_SPEED, 1, MENU_OUTPUT_SPELL));
    speedMenu.add(MI_SPEED_04, new IntMenuItem("Camping Length            %3d sec",  2,  5,/*'m'*/  2, &settingsChanged.campTime, 1, MAX_CAMP_TIME, 1));
    speedMenu.add(MI_SPEED_05, new IntMenuItem("Inn Rest Length           %3d sec",  2,  6,/*'i'*/  0, &settingsChanged.innTime, 1, MAX_INN_TIME, 1));
    speedMenu.add(MI_SPEED_06, new IntMenuItem("Shrine Meditation Length  %3d sec",  2,  7,/*'s'*/  0, &settingsChanged.shrineTime, 1, MAX_SHRINE_TIME, 1));
    speedMenu.add(MI_SPEED_07, new IntMenuItem("Screen Shake Interval     %3d msec", 2,  8,/*'r'*/  2, &settingsChanged.shakeInterval, MIN_SHAKE_INTERVAL, MAX_SHAKE_INTERVAL, 10));
    speedMenu.add(USE_SETTINGS,                "\010 Use These Settings",            2, 11,/*'u'*/  2);
    speedMenu.add(CANCEL,                      "\010 Cancel",                        2, 12,/*'c'*/  2);
    speedMenu.addShortcutKey(CANCEL, ' ');
    speedMenu.setClosesMenu(USE_SETTINGS);
    speedMenu.setClosesMenu(CANCEL);

    /* move the BATTLE DIFFICULTY, DEBUG, and AUTOMATIC ACTIONS settings to "enhancementsOptions" */
    gameplayMenu.setTitle							   ("Enhanced Gameplay Options:", 0, 0);
    gameplayMenu.add(MI_GAMEPLAY_01,    new IntMenuItem("Battle Difficulty          %d", 2,  2,/*'b'*/  0, &settingsChanged.battleDiff, 0, 2, 1));
    gameplayMenu.add(MI_GAMEPLAY_02,   new BoolMenuItem("Fixed Chest Traps          %s", 2,  3,/*'t'*/ 12, &settingsChanged.enhancementsOptions.c64chestTraps));
    gameplayMenu.add(MI_GAMEPLAY_03,   new BoolMenuItem("Gazer Spawns Insects       %s", 2,  4,/*'g'*/  0, &settingsChanged.enhancementsOptions.gazerSpawnsInsects));
    gameplayMenu.add(MI_GAMEPLAY_04,   new BoolMenuItem("Gem View Shows Objects     %s", 2,  5,/*'e'*/  1, &settingsChanged.enhancementsOptions.peerShowsObjects));
    gameplayMenu.add(MI_GAMEPLAY_05,   new BoolMenuItem("Slime Divides              %s", 2,  6,/*'s'*/  0, &settingsChanged.enhancementsOptions.slimeDivides));
    gameplayMenu.add(MI_GAMEPLAY_06,   new BoolMenuItem("Debug Mode (Cheats)        %s", 2,  8,/*'d'*/  0, &settingsChanged.debug)); 
    gameplayMenu.add(USE_SETTINGS,                      "\010 Use These Settings",       2, 11,/*'u'*/  2);
    gameplayMenu.add(CANCEL,                            "\010 Cancel",                   2, 12,/*'c'*/  2);
    gameplayMenu.addShortcutKey(CANCEL, ' ');
    gameplayMenu.setClosesMenu(USE_SETTINGS);
    gameplayMenu.setClosesMenu(CANCEL);

    interfaceMenu.setTitle("Enhanced Interface Options:", 0, 0);
    interfaceMenu.add(MI_INTERFACE_01, new BoolMenuItem("Automatic Actions          %s", 2,  2,/*'a'*/  0, &settingsChanged.shortcutCommands));
    /* "(Open, Jimmy, etc.)" */
    interfaceMenu.add(MI_INTERFACE_02, new BoolMenuItem("Set Active Player          %s", 2,  4,/*'p'*/ 11, &settingsChanged.enhancementsOptions.activePlayer));
    interfaceMenu.add(MI_INTERFACE_03, new BoolMenuItem("Smart 'Enter' Key          %s", 2,  5,/*'e'*/  7, &settingsChanged.enhancementsOptions.smartEnterKey));
    interfaceMenu.add(MI_INTERFACE_04, new BoolMenuItem("Text Colorization          %s", 2,  6,/*'t'*/  0, &settingsChanged.enhancementsOptions.textColorization));
    interfaceMenu.add(MI_INTERFACE_05, new BoolMenuItem("Ultima V Shrines           %s", 2,  7,/*'s'*/  9, &settingsChanged.enhancementsOptions.u5shrines));
    interfaceMenu.add(MI_INTERFACE_06, new BoolMenuItem("Ultima V Spell Mixing      %s", 2,  8,/*'m'*/ 15, &settingsChanged.enhancementsOptions.u5spellMixing));
    interfaceMenu.add(USE_SETTINGS,                     "\010 Use These Settings",       2, 11,/*'u'*/  2);
    interfaceMenu.add(CANCEL,                           "\010 Cancel",                   2, 12,/*'c'*/  2);
    interfaceMenu.addShortcutKey(CANCEL, ' ');
    interfaceMenu.setClosesMenu(USE_SETTINGS);
    interfaceMenu.setClosesMenu(CANCEL);
}

/**
 * Initializes intro state and loads in introduction graphics, text
 * and map data from title.exe.
 */
bool IntroController::init() {

	justInitiatedNewGame = false;

    // sigData is referenced during Titles initialization
    binData = new IntroBinData();
    binData->load();

    if (bSkipTitles)
    {
        // the init() method is called again from within the
        // game via ALT-Q, so return to the menu
        //
        mode = INTRO_MENU;
        beastiesVisible = true;
        beastieOffset = 0;
        zu4_music_play(TRACK_TOWNS);
    }
    else
    {
        // initialize the titles
        initTitles();
        mode = INTRO_TITLES;
        beastiesVisible = false;
        beastieOffset = -32;
    }

    beastie1Cycle = 0;
    beastie2Cycle = 0;

    sleepCycles = 0;
    scrPos = 0;
    objectStateTable = new IntroObjectState[IntroBinData::INTRO_BASETILE_TABLE_SIZE];

    backgroundArea.reinit();
    menuArea.reinit();
    extendedMenuArea.reinit();
    questionArea.reinit();
    mapArea.reinit();

    // only update the screen if we are returning from game mode
    if (bSkipTitles)
        updateScreen();

    return true;
}

bool IntroController::hasInitiatedNewGame()
{
	return this->justInitiatedNewGame;
}

/**
 * Frees up data not needed after introduction.
 */
void IntroController::deleteIntro() {
    delete binData;
    binData = NULL;

    delete [] objectStateTable;
    objectStateTable = NULL;

    imageMgr->freeIntroBackgrounds();
}

unsigned char *IntroController::getSigData() {
    zu4_assert(binData->sigData != NULL, "intro sig data not loaded");
    return binData->sigData;
}

/**
 * Handles keystrokes during the introduction.
 */
bool IntroController::keyPressed(int key) {
    bool valid = true;

    switch (mode) {

    case INTRO_TITLES:
        // the user pressed a key to abort the sequence
        skipTitles();
        break;

    case INTRO_MAP:
        mode = INTRO_MENU;
        updateScreen();
        break;

    case INTRO_MENU:
        switch (key) {
        case 'i':
            errorMessage.erase();
            initiateNewGame();
            break;
        case 'j':
            journeyOnward();
            break;
        case 'r':
            errorMessage.erase();
            mode = INTRO_MAP;
            updateScreen();
            break;
        case 'c': {
            errorMessage.erase();
            // Make a copy of our settings so we can change them
            settingsChanged = settings;
            screenDisableCursor();
            runMenu(&confMenu, &extendedMenuArea, true);
            screenEnableCursor();
            updateScreen();
            break;
        }
        case 'a':
            errorMessage.erase();
            about();
            break;
        case 'q':
            EventHandler::end();
            quit = true;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            zu4_music_play(key - '0');
            break;
        default:
            valid = false;
            break;
        }
        break;

    default:
        zu4_assert(0, "key handler called in wrong mode");
        return true;
    }

    return valid || KeyHandler::defaultHandler(key, NULL);
}

/**
 * Draws the small map on the intro screen.
 */
void IntroController::drawMap() {
    if (0 && sleepCycles > 0) {
        drawMapStatic();
        drawMapAnimated();
        sleepCycles--;
    }
    else {
        unsigned char commandNibble;
        unsigned char dataNibble;

        do {
            commandNibble = binData->scriptTable[scrPos] >> 4;

            switch(commandNibble) {
                /* 0-4 = set object position and tile frame */
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                /* ----------------------------------------------------------
                   Set object position and tile frame
                   Format: yi [t(3); x(5)]
                   i = table index
                   x = x coordinate (5 least significant bits of second byte)
                   y = y coordinate
                   t = tile frame (3 most significant bits of second byte)
                   ---------------------------------------------------------- */
                dataNibble = binData->scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].x = binData->scriptTable[scrPos+1] & 0x1f;
                objectStateTable[dataNibble].y = commandNibble;
                
                // See if the tile id needs to be recalculated 
                if ((binData->scriptTable[scrPos+1] >> 5) >= binData->baseTileTable[dataNibble]->getFrames()) {
                    int frame = (binData->scriptTable[scrPos+1] >> 5) - binData->baseTileTable[dataNibble]->getFrames();
                    objectStateTable[dataNibble].tile = MapTile(binData->baseTileTable[dataNibble]->getId() + 1);
                    objectStateTable[dataNibble].tile.frame = frame;
                }
                else {
                    objectStateTable[dataNibble].tile = MapTile(binData->baseTileTable[dataNibble]->getId());
                    objectStateTable[dataNibble].tile.frame = (binData->scriptTable[scrPos+1] >> 5);
                }
                
                scrPos += 2;
                break;
            case 7:
                /* ---------------
                   Delete object
                   Format: 7i
                   i = table index
                   --------------- */
                dataNibble = binData->scriptTable[scrPos] & 0xf;
                objectStateTable[dataNibble].tile = 0;
                scrPos++;
                break;
            case 8:
                /* ----------------------------------------------
                   Redraw intro map and objects, then go to sleep
                   Format: 8c
                   c = cycles to sleep
                   ---------------------------------------------- */
                drawMapStatic();
                drawMapAnimated();

                /* set sleep cycles */
                sleepCycles = binData->scriptTable[scrPos] & 0xf;
                scrPos++;
                break;
            case 0xf:
                /* -------------------------------------
                   Jump to the start of the script table
                   Format: f?
                   ? = doesn't matter
                   ------------------------------------- */
                scrPos = 0;
                break;
            default:
                /* invalid command */
                scrPos++;
                break;
            }

        } while (commandNibble != 8);
    }
}

void IntroController::drawMapStatic() {
    int x, y;

    // draw unmodified map
    for (y = 0; y < INTRO_MAP_HEIGHT; y++)
        for (x = 0; x < INTRO_MAP_WIDTH; x++)
            mapArea.drawTile(binData->introMap[x + (y * INTRO_MAP_WIDTH)], false, x, y);
}

void IntroController::drawMapAnimated() {
    int i;    

    // draw animated objects
    for (i = 0; i < IntroBinData::INTRO_BASETILE_TABLE_SIZE; i++)
        if (objectStateTable[i].tile != 0)
        {
        	std::vector<MapTile> tiles;
        	tiles.push_back(objectStateTable[i].tile);
        	tiles.push_back(binData->introMap[objectStateTable[i].x + (objectStateTable[i].y * INTRO_MAP_WIDTH)]);
            mapArea.drawTile(tiles, false, objectStateTable[i].x, objectStateTable[i].y);
        }
}

/**
 * Draws the animated beasts in the upper corners of the screen.
 */
void IntroController::drawBeasties() {
    drawBeastie(0, beastieOffset, binData->beastie1FrameTable[beastie1Cycle]);
    drawBeastie(1, beastieOffset, binData->beastie2FrameTable[beastie2Cycle]);
    if (beastieOffset < 0)
        beastieOffset++;
}

/**
 * Animates the "beasties".  The animate intro image is made up frames
 * for the two creatures in the top left and top right corners of the
 * screen.  This function draws the frame for the given beastie on the
 * screen.  vertoffset is used lower the creatures down from the top
 * of the screen.
 */
void IntroController::drawBeastie(int beast, int vertoffset, int frame) {
    char buffer[128];
    int destx;

    zu4_assert(beast == 0 || beast == 1, "invalid beast: %d", beast);

    sprintf(buffer, "beast%dframe%02d", beast, frame);

    destx = beast ? (320 - 48) : 0;
    backgroundArea.draw(buffer, destx, vertoffset);
}

/**
 * Animates the moongate in the tree intro image.  There are two
 * overlays in the part of the image normally covered by the text.  If
 * the frame parameter is "moongate", the moongate overlay is painted
 * over the image.  If frame is "items", the second overlay is
 * painted: the circle without the moongate, but with a small white
 * dot representing the anhk and history book.
 */
void IntroController::animateTree(const string &frame) {
    backgroundArea.draw(frame, 72, 68);
}

/**
 * Draws the cards in the character creation sequence with the gypsy.
 */
void IntroController::drawCard(int pos, int card) {
    static const char *cardNames[] = { 
        "honestycard", "compassioncard", "valorcard", "justicecard",
        "sacrificecard", "honorcard", "spiritualitycard", "humilitycard" 
    };

    zu4_assert(pos == 0 || pos == 1, "invalid pos: %d", pos);
    zu4_assert(card < 8, "invalid card: %d", card);

    backgroundArea.draw(cardNames[card], pos ? 218 : 12, 12);
}

/**
 * Draws the beads in the abacus during the character creation sequence
 */
void IntroController::drawAbacusBeads(int row, int selectedVirtue, int rejectedVirtue) {
    zu4_assert(row >= 0 && row < 7, "invalid row: %d", row);
    zu4_assert(selectedVirtue < 8 && selectedVirtue >= 0, "invalid virtue: %d", selectedVirtue);
    zu4_assert(rejectedVirtue < 8 && rejectedVirtue >= 0, "invalid virtue: %d", rejectedVirtue);
    
    backgroundArea.draw("whitebead", 128 + (selectedVirtue * 9), 24 + (row * 15));
    backgroundArea.draw("blackbead", 128 + (rejectedVirtue * 9), 24 + (row * 15));
}

/**
 * Paints the screen.
 */
void IntroController::updateScreen() {
    screenHideCursor();

    switch (mode) {
    case INTRO_MAP:
        backgroundArea.draw(BKGD_INTRO);
        drawMap();
        drawBeasties();
		// display the profile name if a local profile is being used
		if (useProfile)
			screenTextAt(40-profileName.length(), 24, "%s", profileName.c_str());
        break;

    case INTRO_MENU:
        // draw the extended background for all option screens
        backgroundArea.draw(BKGD_INTRO);
        backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

        // if there is an error message to display, show it
        if (!errorMessage.empty())
        {
            menuArea.textAt(6, 5, "%s", errorMessage.c_str());
            drawBeasties();
            // wait for a couple seconds
            EventHandler::wait_msecs(2000);
            // clear the screen again
            errorMessage.erase();
            backgroundArea.draw(BKGD_INTRO);
            backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
        }

        menuArea.textAt(1,  1, "In another world, in a time to come.");
        menuArea.textAt(14, 3, "Options:");
        menuArea.textAt(10, 5, "%s", menuArea.colorizeString("Return to the view", FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 6, "%s", menuArea.colorizeString("Journey Onward",     FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 7, "%s", menuArea.colorizeString("Initiate New Game",  FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 8, "%s", menuArea.colorizeString("Configure",          FG_YELLOW, 0, 1).c_str());
        menuArea.textAt(10, 9, "%s", menuArea.colorizeString("About",              FG_YELLOW, 0, 1).c_str());
        drawBeasties();

        // draw the cursor last
        screenSetCursorPos(24, 16);
        screenShowCursor();
        break;

    default:
        zu4_assert(0, "bad mode in updateScreen");
    }

    screenUpdateCursor();
}

/**
 * Initiate a new savegame by reading the name, sex, then presenting a
 * series of questions to determine the class of the new character.
 */
void IntroController::initiateNewGame() {
    // disable the screen cursor because a text cursor will now be used
    screenDisableCursor();

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_INTRO);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // display name prompt and read name from keyboard
    menuArea.textAt(3, 3, "By what name shalt thou be known");
    menuArea.textAt(3, 4, "in this world and time?");

    // enable the text cursor after setting it's initial position
    // this will avoid drawing in undesirable areas like 0,0
    menuArea.setCursorPos(11, 7, false);
    menuArea.setCursorFollowsText(true);
    menuArea.enableCursor();

    drawBeasties();

    string nameBuffer = ReadStringController::get(12, &menuArea);
    if (nameBuffer.length() == 0) {
        // the user didn't enter a name
        menuArea.disableCursor();
        screenEnableCursor();
        updateScreen();
        return;
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_INTRO);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // display sex prompt and read sex from keyboard
    menuArea.textAt(3, 3, "Art thou Male or Female?");

    // the cursor is already enabled, just change its position
    menuArea.setCursorPos(28, 3, true);

    drawBeasties();

    SexType sex;
    int sexChoice = ReadChoiceController::get("mf");
    if (sexChoice == 'm')
        sex = SEX_MALE;
    else
        sex = SEX_FEMALE;

    finishInitiateGame(nameBuffer, sex);
}

void IntroController::finishInitiateGame(const string &nameBuffer, SexType sex)
{
    // no more text entry, so disable the text cursor
    menuArea.disableCursor();

    // show the lead up story
    showStory();

    // ask questions that determine character class
    startQuestions();

    // write out save game an segue into game
    SaveGame saveGame;
    SaveGamePlayerRecord avatar;

    u4settings_t *u4settings = zu4_settings_ptr();
    char saveGameFileName[80];
    
    snprintf(saveGameFileName, sizeof(saveGameFileName), "%s%s", u4settings->path, PARTY_SAV_BASE_FILENAME);
    FILE *saveGameFile = fopen(saveGameFileName, "wb");
    if (!saveGameFile) {
        questionArea.disableCursor();
        errorMessage = "Unable to create save game!";
        updateScreen();
        return;
    }

    saveGamePlayerRecordInit(&avatar);
    strcpy(avatar.name, nameBuffer.c_str());
    avatar.sex = sex;
    saveGameInit(&saveGame, &avatar);
    screenHideCursor();
    initPlayers(&saveGame);
    saveGame.food = 30000;
    saveGame.gold = 200;
    saveGame.reagents[REAG_GINSENG] = 3;
    saveGame.reagents[REAG_GARLIC] = 4;
    saveGame.torches = 2;
    //saveGame.write(saveGameFile);
    saveGameWrite(&saveGame, saveGameFile);
    fclose(saveGameFile);

    snprintf(saveGameFileName, sizeof(saveGameFileName), "%s%s", u4settings->path, MONSTERS_SAV_BASE_FILENAME);
    saveGameFile = fopen(saveGameFileName, "wb");
    if (saveGameFile) {
        saveGameMonstersWrite(NULL, saveGameFile);
        fclose(saveGameFile);
    }
    justInitiatedNewGame = true;

    // show the text thats segues into the main game
    showText((string)introGypsy[GYP_SEGUE1]);

    ReadChoiceController pauseController("");
    eventHandler->pushController(&pauseController);
    pauseController.waitFor();

    showText((string)introGypsy[GYP_SEGUE2]);

    eventHandler->pushController(&pauseController);
    pauseController.waitFor();

    // done: exit intro and let game begin
    questionArea.disableCursor();
    EventHandler::setControllerDone();

    return;
}

void IntroController::showStory() {
    ReadChoiceController pauseController("");

    beastiesVisible = false;

    questionArea.setCursorFollowsText(true);

    for (int storyInd = 0; storyInd < 24; storyInd++) {
        if (storyInd == 0)
            backgroundArea.draw(BKGD_TREE);
        else if (storyInd == 3)
            animateTree("moongate");
        else if (storyInd == 5)
            animateTree("items");
        else if (storyInd == 6)
            backgroundArea.draw(BKGD_PORTAL);
        else if (storyInd == 11)
            backgroundArea.draw(BKGD_TREE);
        else if (storyInd == 15)
            backgroundArea.draw(BKGD_OUTSIDE);
        else if (storyInd == 17)
            backgroundArea.draw(BKGD_INSIDE);
        else if (storyInd == 20)
            backgroundArea.draw(BKGD_WAGON);
        else if (storyInd == 21)
            backgroundArea.draw(BKGD_GYPSY);
        else if (storyInd == 23)
            backgroundArea.draw(BKGD_ABACUS);

        showText((string)introText[storyInd]);

        eventHandler->pushController(&pauseController);
        // enable the cursor here to avoid drawing in undesirable locations
        questionArea.enableCursor();
        pauseController.waitFor();
    }
}

/**
 * Starts the gypsys questioning that eventually determines the new
 * characters class.
 */
void IntroController::startQuestions() {
    ReadChoiceController pauseController("");
    ReadChoiceController questionController("ab");

    questionRound = 0;
    initQuestionTree();

    while (1) {
        // draw the abacus background, if necessary
        if (questionRound == 0)
            backgroundArea.draw(BKGD_ABACUS);

        // draw the cards and show the lead up text
        drawCard(0, questionTree[questionRound * 2]);
        drawCard(1, questionTree[questionRound * 2 + 1]);

        questionArea.clear();
        questionArea.textAt(0, 0, "%s", introGypsy[questionRound == 0 ? GYP_PLACES_FIRST : (questionRound == 6 ? GYP_PLACES_LAST : GYP_PLACES_TWOMORE)]);
        questionArea.textAt(0, 1, "%s", introGypsy[GYP_UPON_TABLE]);
        questionArea.textAt(0, 2, "%s and %s.  She says", 
                            introGypsy[questionTree[questionRound * 2] + 4],
                            introGypsy[questionTree[questionRound * 2 + 1] + 4]);
        questionArea.textAt(0, 3, "\"Consider this:\"");

        // wait for a key
        eventHandler->pushController(&pauseController);
        pauseController.waitFor();

        screenEnableCursor();
        // show the question to choose between virtues
        showText(getQuestion(questionTree[questionRound * 2], questionTree[questionRound * 2 + 1]));

        // wait for an answer
        eventHandler->pushController(&questionController);
        int choice = questionController.waitFor();

        // update the question tree
        if (doQuestion(choice == 'a' ? 0 : 1)) {
            return;
        }
    }
}

/**
 * Get the text for the question giving a choice between virtue v1 and
 * virtue v2 (zero based virtue index, starting at honesty).
 */
string IntroController::getQuestion(int v1, int v2) {
    int i = 0;
    int d = 7;

    zu4_assert(v1 < v2, "first virtue must be smaller (v1 = %d, v2 = %d)", v1, v2);

    while (v1 > 0) {
        i += d;
        d--;
        v1--;
        v2--;
    }

    zu4_assert((i + v2 - 1) < 28, "calculation failed");

    return (string)introQuestions[i + v2 - 1];
}

/**
 * Starts the game.
 */
void IntroController::journeyOnward() {
    FILE *saveGameFile;    
    bool validSave = false;

    /*
     * ensure a party.sav file exists, otherwise require user to
     * initiate game
     */
    u4settings_t *u4settings = zu4_settings_ptr();
    char saveGameFileName[80];
    snprintf(saveGameFileName, sizeof(saveGameFileName), "%s%s", u4settings->path, PARTY_SAV_BASE_FILENAME);
    saveGameFile = fopen(saveGameFileName, "rb");
    if (saveGameFile) {
        //SaveGame *saveGame = new SaveGame;
        SaveGame saveGame;

        // Make sure there are players in party.sav --
        // In the Ultima Collection CD, party.sav exists, but does
        // not contain valid info to journey onward
        //saveGame->read(saveGameFile);
        saveGameRead(&saveGame, saveGameFile);
        if (saveGame.members > 0)
            validSave = true;
        //delete saveGame;
        fclose(saveGameFile);
    }
    
    if (!validSave) {
        errorMessage = "Initiate a new game first!";
        updateScreen();
        return;
    }

    EventHandler::setControllerDone();
}

/**
 * Shows an about box.
 */
void IntroController::about() {
    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_INTRO);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    screenHideCursor();
    menuArea.textAt(14, 1, "zu4 %s", VERSION);
    menuArea.textAt(1, 3, "zu4 is free software under the terms");
    menuArea.textAt(1, 4, "of the GNU GPLv2 as published by the");
    menuArea.textAt(1, 5, "FSF. See COPYING for details.");
    menuArea.textAt(2, 7, "Copyright \011 1987, Lord British");
    menuArea.textAt(2, 8, "Copyright \011 2002-2016, xu4 Team");
    menuArea.textAt(2, 9, "Copyright \011 2019-2020, R. Danbrook");
    drawBeasties();

    ReadChoiceController::get("");

    screenShowCursor();
    updateScreen();
}

/**
 * Shows text in the question area.
 */
void IntroController::showText(const string &text) {
    string current = text;
    int lineNo = 0;

    questionArea.clear();
    
    unsigned long pos = current.find("\n");
    while (pos < current.length()) {
        questionArea.textAt(0, lineNo++, "%s", current.substr(0, pos).c_str());
        current = current.substr(pos+1);
        pos = current.find("\n");
    }
    
    /* write the last line (possibly only line) */
    questionArea.textAt(0, lineNo++, "%s", current.substr(0, pos).c_str());
}

/**
 * Run a menu and return when the menu has been closed.  Screen
 * updates are handled by observing the menu.
 */
void IntroController::runMenu(Menu *menu, TextView *view, bool withBeasties) {
    menu->addObserver(this);
    menu->reset();

    // if the menu has an extended height, fill the menu background, otherwise reset the display
    menu->show(view);
    if (withBeasties)
        drawBeasties();

    MenuController menuController(menu, view);
    eventHandler->pushController(&menuController);
    menuController.waitFor();

    // enable the cursor here, after the menu has been established
    view->enableCursor();
    menu->deleteObserver(this);
    view->disableCursor();
}

/**
 * Timer callback for the intro sequence.  Handles animating the intro
 * map, the beasties, etc..
 */
void IntroController::timerFired() {
    screenCycle();
    screenUpdateCursor();

    if (mode == INTRO_TITLES) {
        // Dirty hack to make sure animation backgrounds are black - FIXME
        switch (title->method) {
			case SIGNATURE:
				if (notblanked) {
					zu4_img_fill(title->destImage, 0, 0, title->destImage->w, title->destImage->h, 0, 0, 0, 255);
					notblanked = false;
				}
				break;
			case BAR: case ORIGIN: case PRESENT: case MAP: case SUBTITLE:
				zu4_img_fill(title->destImage, 0, 0, title->destImage->w, title->destImage->h, 0, 0, 0, 255);
				break;
			default: break;
		}
        
        if (updateTitle() == false)
        {
            // setup the map screen
            mode = INTRO_MAP;
            notblanked = false;
            beastiesVisible = true;
            zu4_music_play(TRACK_TOWNS);
            updateScreen();
        }
	}

    if (mode == INTRO_MAP)
        drawMap();

    if (beastiesVisible)
        drawBeasties();

    if (zu4_random(2) && ++beastie1Cycle >= IntroBinData::BEASTIE1_FRAMES)
        beastie1Cycle = 0;
    if (zu4_random(2) && ++beastie2Cycle >= IntroBinData::BEASTIE2_FRAMES)
        beastie2Cycle = 0;
}

/**
 * Update the screen when an observed menu is reset or has an item
 * activated.
 * TODO, reduce duped code.
 */
void IntroController::update(Menu *menu, MenuEvent &event) {
    if (menu == &confMenu)
        updateConfMenu(event);
    else if (menu == &videoMenu)
        updateVideoMenu(event);
    else if (menu == &gfxMenu)
    	updateGfxMenu(event);
    else if (menu == &soundMenu)
        updateSoundMenu(event);
    else if (menu == &inputMenu)
        updateInputMenu(event);
    else if (menu == &speedMenu)
        updateSpeedMenu(event);
    else if (menu == &gameplayMenu)
        updateGameplayMenu(event);
    else if (menu == &interfaceMenu)
        updateInterfaceMenu(event);

    // beasties are always visible on the menus
    drawBeasties();
}

void IntroController::updateConfMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        // show or hide game enhancement options if enhancements are enabled/disabled
        confMenu.getItemById(MI_CONF_GAMEPLAY)->setVisible(settingsChanged.enhancements);
        confMenu.getItemById(MI_CONF_INTERFACE)->setVisible(settingsChanged.enhancements);

        // save settings
        zu4_settings_setdata(settingsChanged);
        zu4_settings_write();

        switch(event.getMenuItem()->getId()) {
        case MI_CONF_VIDEO:
            runMenu(&videoMenu, &extendedMenuArea, true);
            break;
        case MI_VIDEO_CONF_GFX:
        	runMenu(&gfxMenu, &extendedMenuArea, true);
        	break;
        case MI_CONF_SOUND:
            runMenu(&soundMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_INPUT:
            runMenu(&inputMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_SPEED:
            runMenu(&speedMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_GAMEPLAY:
            runMenu(&gameplayMenu, &extendedMenuArea, true);
            break;
        case MI_CONF_INTERFACE:
            runMenu(&interfaceMenu, &extendedMenuArea, true);
            break;
        case CANCEL:
            // discard settings
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateVideoMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            /* save settings (if necessary - FIXME) */
            zu4_settings_setdata(settingsChanged);
            zu4_settings_write();

            /* FIXME: resize images, etc. */
            screenReInit();

            // go back to menu mode
            mode = INTRO_MENU;
            break;
        case MI_VIDEO_CONF_GFX:
        	runMenu(&gfxMenu, &extendedMenuArea, true);
        	break;
        case CANCEL:
            // discard settings
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateGfxMenu(MenuEvent &event)
{
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {


		switch(event.getMenuItem()->getId()) {
		case MI_GFX_RETURN:
			runMenu(&videoMenu, &extendedMenuArea, true);
			break;
		default: break;
		}
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateSoundMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
            case MI_SOUND_01:
                zu4_music_vol((double)settingsChanged.musicVol / MAX_VOLUME);
                break;
            case MI_SOUND_02:
                zu4_snd_vol((double)settingsChanged.soundVol / MAX_VOLUME);
                zu4_snd_play(SOUND_FLEE, true, -1);
                break;
            case USE_SETTINGS:
                // save settings
                zu4_settings_setdata(settingsChanged);
                zu4_settings_write();
                zu4_music_play(TRACK_TOWNS);
                break;
            case CANCEL:
                zu4_music_vol((double)settings.musicVol / MAX_VOLUME);
                zu4_snd_vol((double)settings.soundVol / MAX_VOLUME);
                // discard settings
                settingsChanged = settings;
                break;
            default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateInputMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            // save settings
            zu4_settings_setdata(settingsChanged);
            zu4_settings_write();

            // re-initialize keyboard
            KeyHandler::setKeyRepeat(settingsChanged.keydelay, settingsChanged.keyinterval);
            break;
        case CANCEL:
            // discard settings
            settingsChanged = settings;
            break;
        default: break;
        }    
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateSpeedMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            // save settings
            zu4_settings_setdata(settingsChanged);
            zu4_settings_write();
    
            // re-initialize events
            eventTimerGranularity = (1000 / settings.gameCyclesPerSecond);
            eventHandler->getTimer()->reset(eventTimerGranularity);            
        
            break;
        case CANCEL:
            // discard settings
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateGameplayMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
        case USE_SETTINGS:
            // save settings
            zu4_settings_setdata(settingsChanged);
            zu4_settings_write();
            break;
        case CANCEL:
            // discard settings
            settingsChanged = settings;
            break;
        default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);
}

void IntroController::updateInterfaceMenu(MenuEvent &event) {
    if (event.getType() == MenuEvent::ACTIVATE ||
        event.getType() == MenuEvent::INCREMENT ||
        event.getType() == MenuEvent::DECREMENT) {

        switch(event.getMenuItem()->getId()) {
            case USE_SETTINGS:
                // save settings
                zu4_settings_setdata(settingsChanged);
                zu4_settings_write();
                break;
            case CANCEL:
                // discard settings
                settingsChanged = settings;
                break;
            default: break;
        }
    }

    // draw the extended background for all option screens
    backgroundArea.draw(BKGD_OPTIONS_TOP, 0, 0);
    backgroundArea.draw(BKGD_OPTIONS_BTM, 0, 120);

    // after drawing the menu, extra menu text can be added here
    extendedMenuArea.textAt(2, 3, "  (Open, Jimmy, etc.)");
}

/**
 * Initializes the question tree.  The tree starts off with the first
 * eight entries set to the numbers 0-7 in a random order.
 */
void IntroController::initQuestionTree() {
    int i, tmp, r;

    for (i = 0; i < 8; i++)
        questionTree[i] = i;

    for (i = 0; i < 8; i++) {
        r = zu4_random(8);
        tmp = questionTree[r];
        questionTree[r] = questionTree[i];
        questionTree[i] = tmp;
    }
    answerInd = 8;

    if (questionTree[0] > questionTree[1]) {
        tmp = questionTree[0];
        questionTree[0] = questionTree[1];
        questionTree[1] = tmp;
    }
        
}

/**
 * Updates the question tree with the given answer, and advances to
 * the next round.
 * @return true if all questions have been answered, false otherwise
 */
bool IntroController::doQuestion(int answer) {
    if (!answer)
        questionTree[answerInd] = questionTree[questionRound * 2];
    else
        questionTree[answerInd] = questionTree[questionRound * 2 + 1];
    
    drawAbacusBeads(questionRound, questionTree[answerInd],
        questionTree[questionRound * 2 + ((answer) ? 0 : 1)]);

    answerInd++;
    questionRound++;

    if (questionRound > 6)
        return true;

    if (questionTree[questionRound * 2] > questionTree[questionRound * 2 + 1]) {
        int tmp = questionTree[questionRound * 2];
        questionTree[questionRound * 2] = questionTree[questionRound * 2 + 1];
        questionTree[questionRound * 2 + 1] = tmp;
    }

    return false;
}

/**
 * Build the initial avatar player record from the answers to the
 * gypsy's questions.
 */
void IntroController::initPlayers(SaveGame *saveGame) {
    int i, p;
    static const struct {
        WeaponType weapon;
        ArmorType armor;
        int level, xp, x, y;
    } initValuesForClass[] = {
        { WEAP_STAFF,  ARMR_CLOTH,   2, 125, 231, 136 }, /* CLASS_MAGE */
        { WEAP_SLING,  ARMR_CLOTH,   3, 240,  83, 105 }, /* CLASS_BARD */
        { WEAP_AXE,    ARMR_LEATHER, 3, 205,  35, 221 }, /* CLASS_FIGHTER */
        { WEAP_DAGGER, ARMR_CLOTH,   2, 175,  59,  44 }, /* CLASS_DRUID */
        { WEAP_MACE,   ARMR_LEATHER, 2, 110, 158,  21 }, /* CLASS_TINKER */
        { WEAP_SWORD,  ARMR_CHAIN,   3, 325, 105, 183 }, /* CLASS_PALADIN */
        { WEAP_SWORD,  ARMR_LEATHER, 2, 150,  23, 129 }, /* CLASS_RANGER */
        { WEAP_STAFF,  ARMR_CLOTH,   1,   5, 186, 171 }  /* CLASS_SHEPHERD */
    };
    static const struct {
        const char *name;
        int str, dex, intel;
        SexType sex;
    } initValuesForNpcClass[] = {
        { "Mariah",    9, 12, 20, SEX_FEMALE }, /* CLASS_MAGE */
        { "Iolo",     16, 19, 13, SEX_MALE },   /* CLASS_BARD */
        { "Geoffrey", 20, 15, 11, SEX_MALE },   /* CLASS_FIGHTER */
        { "Jaana",    17, 16, 13, SEX_FEMALE }, /* CLASS_DRUID */
        { "Julia",    15, 16, 12, SEX_FEMALE }, /* CLASS_TINKER */
        { "Dupre",    17, 14, 17, SEX_MALE },   /* CLASS_PALADIN */
        { "Shamino",  16, 15, 15, SEX_MALE },   /* CLASS_RANGER */
        { "Katrina",  11, 12, 10, SEX_FEMALE }  /* CLASS_SHEPHERD */
    };

    saveGame->players[0].klass = static_cast<ClassType>(questionTree[14]);

    //zu4_assert(saveGame->players[0].klass < 8, "bad class: %d", saveGame->players[0].klass);

    saveGame->players[0].weapon = initValuesForClass[saveGame->players[0].klass].weapon;
    saveGame->players[0].armor = initValuesForClass[saveGame->players[0].klass].armor;
    saveGame->players[0].xp = initValuesForClass[saveGame->players[0].klass].xp;
    saveGame->x = initValuesForClass[saveGame->players[0].klass].x;
    saveGame->y = initValuesForClass[saveGame->players[0].klass].y;

    saveGame->players[0].str = 15;
    saveGame->players[0].dex = 15;
    saveGame->players[0].intel = 15;

    for (i = 0; i < VIRT_MAX; i++)
        saveGame->karma[i] = 50;

    for (i = 8; i < 15; i++) {
        saveGame->karma[questionTree[i]] += 5;
        switch (questionTree[i]) {
        case VIRT_HONESTY:
            saveGame->players[0].intel += 3;
            break;
        case VIRT_COMPASSION:
            saveGame->players[0].dex += 3;
            break;
        case VIRT_VALOR:
            saveGame->players[0].str += 3;
            break;
        case VIRT_JUSTICE:
            saveGame->players[0].intel++;
            saveGame->players[0].dex++;
            break;
        case VIRT_SACRIFICE:
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_HONOR:
            saveGame->players[0].intel++;
            saveGame->players[0].str++;
            break;
        case VIRT_SPIRITUALITY:
            saveGame->players[0].intel++;
            saveGame->players[0].dex++;
            saveGame->players[0].str++;
            break;
        case VIRT_HUMILITY:
            /* no stats for you! */
            break;
        }
    }

    PartyMember player(NULL, &saveGame->players[0]);
    saveGame->players[0].hp = saveGame->players[0].hpMax = player.getMaxLevel() * 100;
    saveGame->players[0].mp = player.getMaxMp();

    p = 1;
    for (i = 0; i < VIRT_MAX; i++) {
        player = PartyMember(NULL, &saveGame->players[i]);

        /* Initial setup for party members that aren't in your group yet... */
        if (i != saveGame->players[0].klass) {
            saveGame->players[p].klass = static_cast<ClassType>(i);
            saveGame->players[p].xp = initValuesForClass[i].xp;
            saveGame->players[p].str = initValuesForNpcClass[i].str;
            saveGame->players[p].dex = initValuesForNpcClass[i].dex;
            saveGame->players[p].intel = initValuesForNpcClass[i].intel;
            saveGame->players[p].weapon = initValuesForClass[i].weapon;
            saveGame->players[p].armor = initValuesForClass[i].armor;
            strcpy(saveGame->players[p].name, initValuesForNpcClass[i].name);
            saveGame->players[p].sex = initValuesForNpcClass[i].sex;
            saveGame->players[p].hp = saveGame->players[p].hpMax = initValuesForClass[i].level * 100;
            saveGame->players[p].mp = player.getMaxMp();
            p++;
        }
    }
}


/**
 * Preload map tiles
 */
void IntroController::preloadMap()
{
    int x, y, i;    

    // draw unmodified map
    for (y = 0; y < INTRO_MAP_HEIGHT; y++)
        for (x = 0; x < INTRO_MAP_WIDTH; x++)
            mapArea.loadTile(binData->introMap[x + (y * INTRO_MAP_WIDTH)]);

    // draw animated objects
    for (i = 0; i < IntroBinData::INTRO_BASETILE_TABLE_SIZE; i++) {
        if (objectStateTable[i].tile != 0)
            mapArea.loadTile(objectStateTable[i].tile);
    }
}


//
// Initialize the title elements
//
void IntroController::initTitles()
{
    // add the intro elements
    //          x,  y,   w,  h, method,  delay, duration
    //
    addTitle(  97,  0, 130, 16, SIGNATURE,   1000, 3000 );  // "Lord British"
    addTitle( 148, 17,  24,  4, AND,         1000,  100 );  // "and"
    addTitle(  84, 31, 152,  1, BAR,         1000,  500 );  // <bar>
    addTitle(  86, 21, 148,  9, ORIGIN,      1000,  100 );  // "Origin Systems, Inc."
    addTitle( 133, 33,  54,  5, PRESENT,        0,  100 );  // "present"
    addTitle(  59, 33, 202, 46, TITLE,       1000, 5000 );  // "Ultima IV"
    addTitle(  40, 80, 240, 13, SUBTITLE,    1000,  100 );  // "Quest of the Avatar"
    addTitle(   0, 96, 320, 96, MAP,         1000,  100 );  // the map

    // get the source data for the elements
    getTitleSourceData();

    // reset the iterator
    title = titles.begin();

    // speed up the timer while the intro titles are displayed
    eventHandler->getTimer()->reset(settings.titleSpeedOther);
}


//
// Add the intro element to the element list
//
void IntroController::addTitle(int x, int y, int w, int h, AnimType method, int delay, int duration)
{
    AnimElement data = {
        x, y,               // source x and y
        w, h,               // source width and height
        method,             // render method
        0,                  // animStep
        0,                  // animStepMax
        0,                  // timeBase
        delay,              // delay before rendering begins
        duration,           // total animation time
        NULL,               // storage for the source image
        NULL,               // storage for the animation frame
        std::vector<AnimPlot>()};
    titles.push_back(data);
}


//
// Get the source data for title elements
// that have already been initialized
//
void IntroController::getTitleSourceData()
{
    unsigned int r, g, b, a;        // color values
    unsigned char *srcData;         // plot data

    // The BKGD_INTRO image is assumed to have not been
    // loaded yet.  The unscaled version will be loaded
    // here, and elements of the image will be stored
    // individually.  Afterward, the BKGD_INTRO image
    // will be scaled appropriately.
    ImageInfo *info = imageMgr->get(BKGD_INTRO, true);
    if (!info) {
        zu4_error(ZU4_LOG_ERR, "ERROR 1007: Unable to load the image \"%s\".\t\n\nIs Ultima IV installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_INTRO);
    }

    if (info->width != 320 || info->height != 200)
    {
        // the image appears to have been scaled already
    	zu4_error(ZU4_LOG_WRN, "ERROR 1008: The title image (\"%s\") has been scaled too early!\t\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", BKGD_INTRO);
    }

    // get the transparent color
    transparentColor = RGBA{0, 0, 0, 0};

    // for each element, get the source data
    for (unsigned i=0; i < titles.size(); i++)
    {
        if ((titles[i].method != SIGNATURE)
            && (titles[i].method != BAR))
        {
            // create a place to store the source image
            titles[i].srcImage = zu4_img_create(
                titles[i].rw,
                titles[i].rh);

            // get the source image
            zu4_img_draw_subrect_on(
                titles[i].srcImage, info->image,
                0,
                0,
                titles[i].rx,
                titles[i].ry,
                titles[i].rw,
                titles[i].rh);
        }

        // after getting the srcImage
        switch (titles[i].method)
        {
            case SIGNATURE:
            {
                // PLOT: "Lord British"
                srcData = intro->getSigData();

                RGBA color = RGBA{0, 255, 255, 255};    // cyan for EGA
                uint8_t blue[16] = {255, 250, 226, 226, 210, 194, 161, 161,
                                129,  97,  97,  64,  64,  32,  32,   0};
                uint8_t x = 0;
                uint8_t y = 0;

                while (srcData[titles[i].animStepMax] != 0)
                {
                    x = srcData[titles[i].animStepMax] - 0x4C;
                    y = 0xC0 - srcData[titles[i].animStepMax+1];

                    if (settings.videoType) // Not EGA
                    {
                        // yellow gradient
                        color = RGBA{255, (uint8_t)(y == 1 ? 250 : 255), blue[y], 255};
                    }
                    AnimPlot plot = {
                        x,
                        y,
                        (uint8_t)color.r,
                        (uint8_t)color.g,
                        (uint8_t)color.b,
                        255};
                    titles[i].plotData.push_back(plot);
                    titles[i].animStepMax += 2;
                }
                titles[i].animStepMax = titles[i].plotData.size();
                break;
            }

            case BAR:
            {
                titles[i].animStepMax = titles[i].rw;  // image width
                break;
            }

            case TITLE:
            {
                for (int y=0; y < titles[i].rh; y++)
                {
                    for (int x=0; x < titles[i].rw ; x++)
                    {
                        uint32_t pixIndex = zu4_img_get_pixel(titles[i].srcImage, x, y);
                        r = pixIndex & 0xff;
						g = (pixIndex & 0xff00) >> 8;
						b = (pixIndex & 0xff0000) >> 16;
						a = (pixIndex & 0xff000000) >> 24;
                        if (r || g || b)
                        {
                            AnimPlot plot = {(uint8_t)(x+1), (uint8_t)(y+1), (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a};
                            titles[i].plotData.push_back(plot);
                        }
                    }
                }
                titles[i].animStepMax = titles[i].plotData.size();
                break;
            }

            case MAP:
            {
                // fill the map area with the transparent color
                zu4_img_fill(titles[i].srcImage,
                    8, 8, 304, 80,
                    transparentColor.r,
                    transparentColor.g,
                    transparentColor.b, 255);

                titles[i].animStepMax = 20;
                break;
            }

            default:
            {
                titles[i].animStepMax = titles[i].rh ;  // image height
                break;
            }
        }

        // create the initial animation frame
        titles[i].destImage = zu4_img_create(
            2 + titles[i].rw,
            2 + titles[i].rh);
    }
}


int getTicks()
{
	return SDL_GetTicks();
}

//
// Update the title element, drawing the appropriate frame of animation
//
bool IntroController::updateTitle()
{
    int animStepTarget = 0;

    int timeCurrent = getTicks();
    float timePercent = 0;

    if (title->animStep == 0 && !bSkipTitles)
    {
        if (title->timeBase == 0)
        {
            // reset the base time
            title->timeBase = timeCurrent;
        }
        if (title == titles.begin())
        {
            // clear the screen
            Image *screen = zu4_img_get_screen();
            zu4_img_fill(screen, 0, 0, screen->w, screen->h, 0, 0, 0, 255);
        }
        if (title->method == TITLE)
        {
            // assume this is the first frame of "Ultima IV" and begin sound
            zu4_snd_play(SOUND_TITLE_FADE, true, -1);
        }
    }

    // abort after processing all elements
    if (title == titles.end())
    {
        return false;
    }

    // delay the drawing of this phase
    if ((timeCurrent - title->timeBase) < title->timeDelay)
    {
        return true;
    }

    // determine how much of the animation should have been drawn up until now
    timePercent = float(timeCurrent - title->timeBase - title->timeDelay) / title->timeDuration;
    if (timePercent > 1 || bSkipTitles)
        timePercent = 1;
    animStepTarget = int(title->animStepMax * timePercent);

    // perform the animation
    switch (title->method)
    {
        case SIGNATURE:
        {
            while (animStepTarget > title->animStep)
            {
                // blit the pixel-pair to the src surface
                zu4_img_fill(title->destImage,
                    title->plotData[title->animStep].x,
                    title->plotData[title->animStep].y,
                    2,
                    1,
                    title->plotData[title->animStep].r,
                    title->plotData[title->animStep].g,
                    title->plotData[title->animStep].b, 255);
                title->animStep++;
            }
            break;
        }

        case BAR:
        {
        	RGBA color;
            while (animStepTarget > title->animStep)
            {
                title->animStep++;
                color = RGBA{128, 0, 0, 255}; // dark red for the underline

                // blit bar to the canvas
                zu4_img_fill(title->destImage,
                    1,
                    1,
                    title->animStep,
                    1,
                    color.r,
                    color.g,
                    color.b, 255);
            }
            break;
        }

        case AND:
        {
            // blit the entire src to the canvas
            zu4_img_draw_on(title->destImage, title->srcImage, 1, 1);
            title->animStep = title->animStepMax;
            break;
        }

        case ORIGIN:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            // blit src to the canvas one row at a time, bottom up
            zu4_img_draw_subrect_on(
                title->destImage, title->srcImage,
                1,
                title->destImage->h - 1 - title->animStep,
                0,
                0,
                title->srcImage->w,
                title->animStep);
            break;
        }

        case PRESENT:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            // blit src to the canvas one row at a time, top down
            zu4_img_draw_subrect_on(
                title->destImage, title->srcImage,
                1,
                1,
                0,
                title->srcImage->h - title->animStep,
                title->srcImage->w,
                title->animStep);
            break;
        }

        case TITLE:
        {
            // blit src to the canvas in a random pixelized manner
            title->animStep = animStepTarget;

            random_shuffle(title->plotData.begin(), title->plotData.end());
            zu4_img_fill(title->destImage, 1, 1, title->rw, title->rh, 0, 0, 0, 255);

            // @TODO: animStepTarget (for this loop) should not exceed
            // half of animStepMax.  If so, instead draw the entire
            // image, and then black out the necessary pixels.
            // this should speed the loop up at the end
            for (int i=0; i < animStepTarget; ++i)
            {
                zu4_img_set_pixel(title->destImage,
                    title->plotData[i].x,
                    title->plotData[i].y,
                    title->plotData[i].r |
                    title->plotData[i].g << 8 |
                    title->plotData[i].b << 16 |
                    title->plotData[i].a << 24);
            }

            // cover the "present" area with the transparent color
            zu4_img_fill(title->destImage,
                75, 1, 54, 5,
                transparentColor.r,
                transparentColor.g,
                transparentColor.b, 255);
            break;
        }

        case SUBTITLE:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            // blit src to the canvas one row at a time, center out
            int y = int(title->rh / 2) - title->animStep + 1;
            zu4_img_draw_subrect_on(
                title->destImage, title->srcImage,
                1,
                y+1,
                0,
                y,
                title->srcImage->w,
                1 + ((title->animStep - 1) * 2));
            break;
        }

        case MAP:
        {
            if (bSkipTitles)
                title->animStep = title->animStepMax;
            else
            {
                title->animStep++;
                title->timeDelay = getTicks() - title->timeBase + 100;
            }

            int step = (title->animStep == title->animStepMax ? title->animStepMax - 1 : title->animStep);

            // blit src to the canvas one row at a time, center out
            zu4_img_draw_subrect_on(
                title->destImage, title->srcImage,
                153-(step*8),
                1,
                0,
                0,
                (step+1) * 8,
                title->srcImage->h);
            zu4_img_draw_subrect_on(
                title->destImage, title->srcImage,
                161,
                1,
                312-(step*8),
                0,
                (step+1) * 8,
                title->srcImage->h);


            // create a destimage for the map tiles
            int newtime = getTicks();
            if (newtime > title->timeDuration + 250/4)
            {
                // grab the map from the screen
                Image *screen = zu4_img_get_screen();

                // draw the updated map display
                intro->drawMapStatic();

                zu4_img_draw_subrect_on(
                    title->srcImage, screen,
                    8,
                    8,
                    8,
                    13*8,
                    38*8,
                    10*8);

                title->timeDuration = newtime + 250/4;
            }

            zu4_img_draw_subrect_on(
                title->destImage, title->srcImage,
                161 - (step * 8),
                9,
                160 - (step * 8),
                8,
                (step * 2) * 8,
                (10 * 8));

            break;
        }
    }

    // draw the titles
    drawTitle();

    // if the animation for this title has completed,
    // move on to the next title
    if (title->animStep >= title->animStepMax)
    {
        // free memory that is no longer needed
        compactTitle();
        title++;

        if (title == titles.end())
        {
            // reset the timer to the pre-titles granularity
            eventHandler->getTimer()->reset(eventTimerGranularity);

            // make sure the titles only appear when the app first loads
            bSkipTitles = true;

            return false;
        }

        if (title->method == TITLE)
        {
            // assume this is "Ultima IV" and pre-load sound
//            soundLoad(SOUND_TITLE_FADE);
            eventHandler->getTimer()->reset(settings.titleSpeedRandom);
        }
        else if (title->method == MAP)
        {
            eventHandler->getTimer()->reset(settings.titleSpeedOther);
        }
        else
        {
            eventHandler->getTimer()->reset(settings.titleSpeedOther);
        }
    }

    return true;
}


//
// The title element has finished drawing all frames, so
// delete, remove, or free data that is no longer needed
//
void IntroController::compactTitle()
{
    if (title->srcImage)
    {
        zu4_img_free(title->srcImage);
        title->srcImage = NULL;
    }
    title->plotData.clear();
}


//
// Scale the animation canvas, then draw it to the screen
//
void IntroController::drawTitle()
{
    Image *t = zu4_img_dup(title->destImage);

    zu4_img_draw_subrect(t,
        title->rx,    // dest x, y
        title->ry,
        1,              // src x, y, w, h
        1,
        title->rw,
        title->rh);

    zu4_img_free(t);
    t = NULL;
}


//
// skip the remaining titles
//
void IntroController::skipTitles()
{
    bSkipTitles = true;
    zu4_snd_stop();
}
