/*
 * $Id: dialogueloader_lb.cpp 3012 2012-03-05 21:24:06Z twschulz $
 */

#include <string>

#include "context.h"
#include "conversation.h"
#include "dialogueloader_lb.h"
#include "player.h"
#include "u4file.h"

Response *lordBritishGetHelp(const DynamicResponse *resp);
Response *lordBritishGetIntro(const DynamicResponse *resp);

DialogueLoader* U4LBDialogueLoader::instance = DialogueLoader::registerLoader(new U4LBDialogueLoader, "application/x-u4lbtlk");

/**
 * A special case dialogue loader for Lord British.  Loads most of the
 * keyword/responses from a hardcoded location in avatar.exe.  The
 * "help" response is a special case that changes based on the
 * current party status.
 */
Dialogue* U4LBDialogueLoader::load(void *source) {
    U4FILE *avatar = u4fopen("avatar.exe");
    if (!avatar)
        return NULL;

    char *lbKeywords[24];
    zu4_read_strtable(avatar, 87581, (char**)lbKeywords, 24);

    /* There's a \0 in the 19th string so we get a
       spurious 20th entry */
    char *lbText[25];
    zu4_read_strtable(avatar, 87754, (char**)lbText, 25);

    char *lb20 = lbText[20];
    for (int i = 20; i < 24; i++)
        lbText[i] = lbText[i+1];
    lbText[24] = lb20; // Make sure it can be freed

    Dialogue *dlg = new Dialogue();
    dlg->setTurnAwayProb(0);

    dlg->setName("Lord British");
    dlg->setPronoun("He");
    dlg->setPrompt("What else?\n");
    Response *intro = new DynamicResponse(&lordBritishGetIntro);
    dlg->setIntro(intro);
    dlg->setLongIntro(intro);
    dlg->setDefaultAnswer(new Response("\nHe says: I\ncannot help thee\nwith that.\n"));

    for (unsigned i = 0; i < 24; i++) {
        dlg->addKeyword(lbKeywords[i], new Response(lbText[i]));
    }

    /* since the original game files are a bit sketchy on the 'abyss' keyword,
       let's handle it here just to be safe :) */
    dlg->addKeyword("abyss",
                    new Response("\n\n\n\n\nHe says:\nThe Great Stygian Abyss is the darkest pocket of evil "
                                 "remaining in Britannia!\n\n\n\n\nIt is said that in the deepest recesses of "
                                 "the Abyss is the Chamber of the Codex!\n\n\n\nIt is also said that only one "
                                 "of highest Virtue may enter this Chamber, one such as an Avatar!!!\n"));

    Response *heal = new Response("\n\n\n\n\n\nHe says: I am\nwell, thank ye.");
    heal->add(ResponsePart::HEALCONFIRM);
    dlg->addKeyword("heal", heal);

    Response *bye;
    if (c->party->size() > 1)
        bye = new Response("Lord British says: Fare thee well my friends!");
    else
        bye = new Response("Lord British says: Fare thee well my friend!");
    bye->add(ResponsePart::STOPMUSIC);
    bye->add(ResponsePart::END);
    dlg->addKeyword("bye", bye);
    dlg->addKeyword("", bye);

    dlg->addKeyword("help", new DynamicResponse(&lordBritishGetHelp));

    for (int i = 0; i < 24; i++) { free(lbKeywords[i]); }
    for (int i = 0; i < 25; i++) { free(lbText[i]); }

    return dlg;
}

/**
 * Generate the appropriate response when the player asks Lord British
 * for help.  The help text depends on the current party status; when
 * one quest item is complete, Lord British provides some direction to
 * the next one.
 */
Response *lordBritishGetHelp(const DynamicResponse *resp) {
    int v;
    bool fullAvatar, partialAvatar;
    std::string text;

    /*
     * check whether player is full avatar (in all virtues) or partial
     * avatar (in at least one virtue)
     */
    fullAvatar = true;
    partialAvatar = false;
    for (v = 0; v < VIRT_MAX; v++) {
        fullAvatar &= (c->saveGame->karma[v] == 0);
        partialAvatar |= (c->saveGame->karma[v] == 0);
    }

    if (c->saveGame->moves <= 1000) {
        text = "To survive in this hostile land thou must first know thyself! Seek ye to master thy weapons and thy magical ability!\n"
               "\nTake great care in these thy first travels in Britannia.\n"
               "\nUntil thou dost well know thyself, travel not far from the safety of the townes!\n";
    }

    else if (c->saveGame->members == 1) {
        text = "Travel not the open lands alone. There are many worthy people in the diverse townes whom it would be wise to ask to Join thee!\n"
               "\nBuild thy party unto eight travellers, for only a true leader can win the Quest!\n";
    }

    else if (c->saveGame->runes == 0) {
        text = "Learn ye the paths of virtue. Seek to gain entry unto the eight shrines!\n"
               "\nFind ye the Runes, needed for entry into each shrine, and learn each chant or \"Mantra\" used to focus thy meditations.\n"
               "\nWithin the Shrines thou shalt learn of the deeds which show thy inner virtue or vice!\n"
               "\nChoose thy path wisely for all thy deeds of good and evil are remembered and can return to hinder thee!\n";
    }

    else if (!partialAvatar) {
        text = "Visit the Seer Hawkwind often and use his wisdom to help thee prove thy virtue.\n"
               "\nWhen thou art ready, Hawkwind will advise thee to seek the Elevation unto partial Avatarhood in a virtue.\n"
               "\nSeek ye to become a partial Avatar in all eight virtues, for only then shalt thou be ready to seek the codex!\n";
    }

    else if (c->saveGame->stones == 0) {
        text = "Go ye now into the depths of the dungeons. Therein recover the 8 colored stones from the altar pedestals in the halls of the dungeons.\n"
               "\nFind the uses of these stones for they can help thee in the Abyss!\n";
    }

    else if (!fullAvatar) {
        text = "Thou art doing very well indeed on the path to Avatarhood! Strive ye to achieve the Elevation in all eight virtues!\n";
    }

    else if ((c->saveGame->items & ITEM_BELL) == 0 ||
             (c->saveGame->items & ITEM_BOOK) == 0 ||
             (c->saveGame->items & ITEM_CANDLE) == 0) {
        text = "Find ye the Bell, Book and Candle!  With these three things, one may enter the Great Stygian Abyss!\n";
    }

    else if ((c->saveGame->items & ITEM_KEY_C) == 0 ||
             (c->saveGame->items & ITEM_KEY_L) == 0 ||
             (c->saveGame->items & ITEM_KEY_T) == 0) {
        text = "Before thou dost enter the Abyss thou shalt need the Key of Three Parts, and the Word of Passage.\n"
               "\nThen might thou enter the Chamber of the Codex of Ultimate Wisdom!\n";
    }

    else {
        text = "Thou dost now seem ready to make the final journey into the dark Abyss! Go only with a party of eight!\n"
               "\nGood Luck, and may the powers of good watch over thee on this thy most perilous endeavor!\n"
               "\nThe hearts and souls of all Britannia go with thee now. Take care, my friend.\n";
    }

    return new Response(std::string("He says: ") + text);
}

Response *lordBritishGetIntro(const DynamicResponse *resp) {
    Response *intro = new Response("");
    intro->add(ResponsePart::STARTMUSIC_LB);

    if (c->saveGame->lbintro) {
        if (c->saveGame->members == 1) {
            intro->add(std::string("\n\n\nLord British\nsays:  Welcome\n") +
                       c->party->member(0)->getName() + "!\n\n");
        }
        else if (c->saveGame->members == 2) {
            intro->add(std::string("\n\nLord British\nsays:  Welcome\n") +
                       c->party->member(0)->getName() +
                       " and thee also " +
                       c->party->member(1)->getName() +
                       "!\n\n");
        }
        else {
            intro->add(std::string("\n\n\nLord British\nsays:  Welcome\n") +
                       c->party->member(0)->getName() +
                       " and thy\nworthy\nAdventurers!\n\n");
        }

        // Lord British automatically adds "What would thou ask of me?"

        // Check levels here, just like the original!
        intro->add(ResponsePart::ADVANCELEVELS);
    }

    else {
        intro->add(std::string("\n\n\nLord British rises and says: At long last!\n") +
                   c->party->member(0)->getName() +
                   " thou hast come!  We have waited such a long, long time...\n"
                   "\n\nLord British sits and says: A new age is upon Britannia. The great evil Lords are gone but our people lack direction and purpose in their lives...\n\n\n"
                   "A champion of virtue is called for. Thou may be this champion, but only time shall tell.  I will aid thee any way that I can!\n\n"
                   "How may I help thee?\n");
        c->saveGame->lbintro = 1;
    }

    return intro;
}
