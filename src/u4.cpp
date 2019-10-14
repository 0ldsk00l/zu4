/*
 * $Id: u4.cpp 3079 2014-07-30 01:10:56Z darren_janeczek $
 */

/** \mainpage xu4 Main Page
 *
 * \section intro_sec Introduction
 *
 * intro stuff goes here...
 */

#include "u4.h"
#include <cstring>
#include "error.h"
#include "event.h"
#include "game.h"
#include "intro.h"
#include "music.h"
#include "person.h"
#include "progress_bar.h"
#include "random.h"
#include "screen.h"
#include "settings.h"
#include "sound.h"
#include "tileset.h"
#include "utils.h"

bool verbose = false;
bool quit = false;
bool useProfile = false;
string profileName = "";

using namespace std;

int main(int argc, char *argv[]) {
    if (!u4fopen("AVATAR.EXE"))
	{
        xu4_error(XU4_LOG_ERR, 	"xu4 requires the PC version of Ultima IV to be present. "
        			"It must either be in the same directory as the xu4 executable, "
        			"or in a subdirectory named \"ultima4\"."
        			"\n\nThis can be achieved by downloading \"UltimaIV.zip\" from www.ultimaforever.com"
        			"\n - Extract the contents of UltimaIV.zip"
        			"\n - Copy the \"ultima4\" folder to your xu4 executable location."
        			"\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/");
	}

	unsigned int i;
    int skipIntro = 0;


    /*
     * if the -p or -profile arguments are passed to the application,
     * they need to be identified before the settings are initialized.
     */
    for (i = 1; i < (unsigned int)argc; i++) {
        if (((strcmp(argv[i], "-p") == 0)
          || (strcmp(argv[i], "-profile") == 0)
          || (strcmp(argv[i], "--profile") == 0))
                && (unsigned int)argc > i + 1) {
            // when grabbing the profile name:
            // 1. trim leading whitespace
            // 2. truncate the string at 20 characters
            // 3. then strip any trailing whitespace
            profileName = argv[i+1];
            profileName = profileName.erase(0,profileName.find_first_not_of(' '));
            profileName.resize(20, ' ');
            profileName = profileName.erase(profileName.find_last_not_of(' ')+1);

            // verify that profileName is valid, otherwise do not use the profile
            if (!profileName.empty()) {
                useProfile = true;
            }
            i++;
            break;
        }
    }

    /* initialize the settings */
    settings.init(useProfile, profileName);

    /* update the settings based upon command-line arguments */
    for (i = 1; i < (unsigned int)argc; i++) {
        if (strcmp(argv[i], "-s") == 0
               || strcmp(argv[i], "-scale") == 0
               || strcmp(argv[i], "--scale") == 0)
        {
            if ((unsigned int)argc > i + 1)
            {
                settings.scale = strtoul(argv[i+1], NULL, 0);
                i++;
            }
            else
                xu4_error(XU4_LOG_ERR, "%s is invalid alone: Requires a number for input. See --help for more detail.\n", argv[i]);


        }
        else if ( strcmp(argv[i], "-p") == 0
                || strcmp(argv[i], "-profile") == 0
                || strcmp(argv[i], "--profile") == 0)
        {
            // do nothing
            if ((unsigned int)argc > i + 1)
                i++;
            else
                xu4_error(XU4_LOG_ERR, "%s is invalid alone: Requires a string as input. See --help for more detail.\n", argv[i]);

        }
        else if (strcmp(argv[i], "-i") == 0
              || strcmp(argv[i], "-skipintro") == 0
              || strcmp(argv[i], "--skip-intro") == 0)
        {
                skipIntro = 1;
        }
        else if (strcmp(argv[i], "-v") == 0
              || strcmp(argv[i], "-verbose") == 0
              || strcmp(argv[i], "--verbose") == 0)
        {
            verbose = true;
        }
        else if (strcmp(argv[i], "-f") == 0
              || strcmp(argv[i], "-fullscreen") == 0
              || strcmp(argv[i], "--fullscreen") == 0)
        {
            settings.fullscreen = 1;
        }
        else if (strcmp(argv[i], "-q") == 0
              || strcmp(argv[i], "-quiet") == 0
              || strcmp(argv[i], "--quiet") == 0)
        {
            settings.musicVol = 0;
            settings.soundVol = 0;
        }
        else if (strcmp(argv[i], "-h") == 0
              || strcmp(argv[i], "-help") == 0
              || strcmp(argv[i], "--help") == 0)
        {
            printf("xu4: Ultima IV Recreated\n");
            printf("v%s\n\n", VERSION);

            printf("-v, --verbose		Runs xu4 in verbose mode. Increased console output.\n");
            printf("-q, --quiet		Sets all audio volume to zero.\n");
            printf("-f, --fullscreen	Runs xu4 in fullscreen mode.\n");
            printf("-i, --skip-intro	Skips the intro and loads the last savegame.\n");

            printf("\n-s <int>,\n");
            printf("--scale <int>		Used to specify scaling options.\n");
            printf("-p <string>,\n");
            printf("--profile <string>	Used to pass extra arguments to the program.\n");
            printf("--filter <string>	Used to specify filtering options.\n");

            printf("\n-h, --help		Prints this message.\n");

            return 0;
        }
        else
            xu4_error(XU4_LOG_ERR, "Unrecognized argument: %s\n\nUse --help for a list of supported arguments.", argv[i]);

    }

    xu4_srandom();

    screenInit();
    ProgressBar pb((320/2) - (200/2), (200/2), 200, 10, 0, (skipIntro ? 4 : 7));
    pb.setBorderColor(240, 240, 240);
    pb.setColor(0, 0, 128);
    pb.setBorderWidth(1);

    screenTextAt(15, 11, "Loading...");
    screenRedrawScreen();
    ++pb;

    xu4_music_init();
    xu4_snd_init();
    ++pb;

    Tileset::loadAll();
    ++pb;

    creatureMgr->getInstance();
    ++pb;

    intro = new IntroController();
    
    if (!skipIntro)
    {
        /* do the intro */
        intro->init();
        ++pb;

        intro->preloadMap();
        ++pb;

        eventHandler->pushController(intro);
        eventHandler->run();
        eventHandler->popController();
        intro->deleteIntro();
    }

    eventHandler->setControllerDone(false);
    if (quit) { return 0; }

    /* play the game! */
    game = new GameController();
    game->init();

    eventHandler->pushController(game);
    eventHandler->run();
    eventHandler->popController();

    Tileset::unloadAll();

    xu4_snd_deinit();
    xu4_music_deinit();
    screenDelete();

    return 0;
}
