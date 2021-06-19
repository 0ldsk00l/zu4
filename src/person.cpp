/*
 * $Id: person.cpp 3064 2014-07-19 15:44:38Z darren_janeczek $
 */

#include <string.h>
#include <vector>

#include "u4.h"
#include "person.h"
#include "city.h"
#include "context.h"
#include "conversation.h"
#include "error.h"
#include "event.h"
#include "game.h"   // Included for ReadPlayerController
#include "io.h"
#include "location.h"
#include "music.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "settings.h"
#include "stats.h"
#include "random.h"
#include "types.h"
#include "u4file.h"
#include "utils.h"
#include "script.h"

using namespace std;

int chars_needed(const char *s, int columnmax, int linesdesired, int *real_lines);

/**
 * Returns true of the object that 'punknown' points
 * to is a person object
 */ 
bool isPerson(Object *punknown) {
    Person *p;
    if ((p = dynamic_cast<Person*>(punknown)) != NULL)
        return true;
    else
        return false;
}

/**
 * Splits a string into substrings, divided by the charactars in
 * separators.  Multiple adjacent seperators are treated as one.
 */
static std::vector<string> split(const string &s, const string &separators) {
    std::vector<string> result;
    string current;

    for (unsigned i = 0; i < s.length(); i++) {
        if (separators.find(s[i]) != string::npos) {
            if (current.length() > 0)
                result.push_back(current);
            current.erase();
        } else
            current += s[i];
    }

    if (current.length() > 0)
        result.push_back(current);

    return result;
}

/**
 * Splits a piece of response text into screen-sized chunks.
 */
list<string> replySplit(const string &text) {
    string str = text;
    int pos, real_lines;
    list<string> reply;

    /* skip over any initial newlines */
    if ((pos = str.find("\n\n")) == 0)
        str = str.substr(pos+1);
    
    unsigned int num_chars = chars_needed(str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines);
    
    /* we only have one chunk, no need to split it up */
    unsigned int len = str.length();
    if (num_chars == len)
        reply.push_back(str);
    else {
        string pre = str.substr(0, num_chars);        

        /* add the first chunk to the list */
        reply.push_back(pre);
        /* skip over any initial newlines */
        if ((pos = str.find("\n\n")) == 0)
            str = str.substr(pos+1);

        while (num_chars != str.length()) {
            /* go to the rest of the text */
            str = str.substr(num_chars);
            /* skip over any initial newlines */
            if ((pos = str.find("\n\n")) == 0)
                str = str.substr(pos+1);

            /* find the next chunk and add it */
            num_chars = chars_needed(str.c_str(), TEXT_AREA_W, TEXT_AREA_H, &real_lines);
            pre = str.substr(0, num_chars);            

            reply.push_back(pre);
        }
    }
    
    return reply;
}

Person::Person(MapTile tile) : Creature(tile) {
    start = {0, 0, 0};
    setType(Object::PERSON);
    dialogue = NULL;
    npcType = NPC_EMPTY;
}

Person::Person(const Person *p) : Creature(p->tile) {
    *this = *p;
}

bool Person::canConverse() const {
    return isVendor() || dialogue != NULL;
}

bool Person::isVendor() const {
    return
        npcType >= NPC_VENDOR_WEAPONS &&
        npcType <= NPC_VENDOR_STABLE;
}

string Person::getName() const {
    if (dialogue)
        return dialogue->getName();
    else if (npcType == NPC_EMPTY)
        return Creature::getName();
    else
        return "(unnamed person)";
}

void Person::goToStartLocation() {
    setCoords(start);
}

void Person::setDialogue(Dialogue *d) {
    dialogue = d;
    if (tile.getTileType()->getName() == "beggar")
        npcType = NPC_TALKER_BEGGAR;
    else if (tile.getTileType()->getName() == "guard")
        npcType = NPC_TALKER_GUARD;
    else
        npcType = NPC_TALKER;
}

void Person::setNpcType(PersonNpcType t) {
    npcType = t;
    zu4_assert(!isVendor() || dialogue == NULL, "vendor has dialogue");
}

list<string> Person::getConversationText(Conversation *cnv, const char *inquiry) {
    string text;

    /*
     * a convsation with a vendor
     */
    if (isVendor()) {
        static const string ids[] = { 
            "Weapons", "Armor", "Food", "Tavern", "Reagents", "Healer", "Inn", "Guild", "Stable"
        };
        Script *script = cnv->script;

        /**
         * We aren't currently running a script, load the appropriate one!
         */ 
        if (cnv->state == Conversation::INTRO) {
            // unload the previous script if it wasn't already unloaded
            if (script->getState() != Script::STATE_UNLOADED)
                script->unload();
            script->load("vendorScript.xml", ids[npcType - NPC_VENDOR_WEAPONS], "vendor", c->location->map->getName());
            script->run("intro");       

            while (script->getState() != Script::STATE_DONE) {
                // Gather input for the script
                if (script->getState() == Script::STATE_INPUT) {
                    switch(script->getInputType()) {
                    case Script::INPUT_CHOICE: {
                        const string &choices = script->getChoices();
                        // Get choice
                        char val = ReadChoiceController::get(choices);
                        if (isspace(val) || val == '\033')
                            script->unsetVar(script->getInputName());
                        else {
                            string s_val;
                            s_val.resize(1);
                            s_val[0] = val;
                            script->setVar(script->getInputName(), s_val);
                        }
                    } break;

                    case Script::INPUT_KEYPRESS:
                        ReadChoiceController::get(" \015\033");
                        break;
                        
                    case Script::INPUT_NUMBER: {
                        int val = ReadIntController::get(script->getInputMaxLen(), TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
                        script->setVar(script->getInputName(), val);
                    } break;

                    case Script::INPUT_STRING: {
                        string str = ReadStringController::get(script->getInputMaxLen(), TEXT_AREA_X + c->col, TEXT_AREA_Y + c->line);
                        if (str.size()) {
                            transform(str.begin(), str.end(), str.begin(), ::tolower);
                            script->setVar(script->getInputName(), str);
                        }
                        else script->unsetVar(script->getInputName());
                    } break;                    
                    
                    case Script::INPUT_PLAYER: {
                        ReadPlayerController getPlayerCtrl;
                        eventHandler->pushController(&getPlayerCtrl);
                        int player = getPlayerCtrl.waitFor();
                        if (player != -1) {
                            char buffer[16];
                            snprintf(buffer, sizeof(buffer), "%d", player + 1);
                            script->setVar(script->getInputName(), (string)buffer);
                        }
                        else script->unsetVar(script->getInputName());
                    } break;

                    default: break;
                    } // } switch

                    // Continue running the script!
                    c->line++;
                    script->_continue();
                } // } if 
            } // } while
        }
        
        // Unload the script
        script->unload();
        cnv->state = Conversation::DONE;
    }

    /*
     * a conversation with a non-vendor
     */
    else {
        text = "\n\n\n";

        switch (cnv->state) {
        case Conversation::INTRO:
            text = getIntro(cnv);
            break;

        case Conversation::TALK:
            text += getResponse(cnv, inquiry) + "\n";
            break;

        case Conversation::CONFIRMATION:
            zu4_assert(npcType == NPC_LORD_BRITISH, "invalid state: %d", cnv->state);
            text += lordBritishGetQuestionResponse(cnv, inquiry);
            break;

        case Conversation::ASK:
        case Conversation::ASKYESNO:
            zu4_assert(npcType != NPC_HAWKWIND, "invalid state for hawkwind conversation");            
            text += talkerGetQuestionResponse(cnv, inquiry) + "\n";
            break;

        case Conversation::GIVEBEGGAR:
            zu4_assert(npcType == NPC_TALKER_BEGGAR, "invalid npc type: %d", npcType);
            text = beggarGetQuantityResponse(cnv, inquiry);
            break;

        case Conversation::FULLHEAL:
        case Conversation::ADVANCELEVELS:
            /* handled elsewhere */
            break;

        default:
            zu4_assert(0, "invalid state: %d", cnv->state);
        }
    }

    return replySplit(text);
}

/**
 * Get the prompt shown after each reply.
 */
string Person::getPrompt(Conversation *cnv) {
    if (isVendor())
        return "";

    string prompt;
    if (cnv->state == Conversation::ASK)
        prompt = getQuestion(cnv);
    else if (cnv->state == Conversation::GIVEBEGGAR)
        prompt = "How much? ";
    else if (cnv->state == Conversation::CONFIRMATION)
        prompt = "\n\nHe asks: Art thou well?";
    else if (cnv->state != Conversation::ASKYESNO)
        prompt = dialogue->getPrompt();

    return prompt;
}

/**
 * Returns the valid keyboard choices for a given conversation.
 */
const char *Person::getChoices(Conversation *cnv) {
    if (isVendor())
        return cnv->script->getChoices().c_str();
    
    switch (cnv->state) {    
    case Conversation::CONFIRMATION:
    case Conversation::CONTINUEQUESTION:
        return "ny\015 \033";

    case Conversation::PLAYER:
        return "012345678\015 \033";

    default:
        zu4_assert(0, "invalid state: %d", cnv->state);
    }

    return NULL;
}

string Person::getIntro(Conversation *cnv) {
    if (npcType == NPC_EMPTY) {
        cnv->state = Conversation::DONE;
        return string("Funny, no\nresponse!\n");
    }

    // As far as I can tell, about 50% of the time they tell you their
    // name in the introduction
    Response *intro;
    if (zu4_random(2) == 0)
        intro = dialogue->getIntro();
    else
        intro = dialogue->getLongIntro();

    cnv->state = Conversation::TALK;
    string text = processResponse(cnv, intro);

    return text;
}

string Person::processResponse(Conversation *cnv, Response *response) {
    string text;
    const vector<ResponsePart> &parts = response->getParts();
    for (vector<ResponsePart>::const_iterator i = parts.begin(); i != parts.end(); i++) {

        // check for command triggers
        if (i->isCommand())
            runCommand(cnv, *i);

        // otherwise, append response part to reply
        else
            text += *i;
    }
    return text;
}

void Person::runCommand(Conversation *cnv, const ResponsePart &command) {
    if (command == ResponsePart::ASK) {
        cnv->question = dialogue->getQuestion();
        cnv->state = Conversation::ASK;
    }
    else if (command == ResponsePart::END) {
        cnv->state = Conversation::DONE;
    }
    else if (command == ResponsePart::ATTACK) {
        cnv->state = Conversation::ATTACK;
    }
    else if (command == ResponsePart::BRAGGED) {
        c->party->adjustKarma(KA_BRAGGED);
    }
    else if (command == ResponsePart::HUMBLE) {
        c->party->adjustKarma(KA_HUMBLE);
    }
    else if (command == ResponsePart::ADVANCELEVELS) {
        cnv->state = Conversation::ADVANCELEVELS;
    }
    else if (command == ResponsePart::HEALCONFIRM) {
        cnv->state = Conversation::CONFIRMATION;
    }
    else if (command == ResponsePart::STARTMUSIC_LB) {
        zu4_music_play(TRACK_RULEBRIT);
    }
    else if (command == ResponsePart::STARTMUSIC_HW) {
        zu4_music_play(TRACK_SHOPPING);
    }
    else if (command == ResponsePart::STOPMUSIC) {
        zu4_music_play(c->location->map->music);
    }
    else if (command == ResponsePart::HAWKWIND) {
        c->party->adjustKarma(KA_HAWKWIND);
    }
    else {
        zu4_assert(0, "unknown command trigger in dialogue response: %s\n", string(command).c_str());
    }
} 

string Person::getResponse(Conversation *cnv, const char *inquiry) {
    string reply;
    Virtue v;
    const ResponsePart &action = dialogue->getAction();

    reply = "\n";
    
    /* Does the person take action during the conversation? */
    if (action == ResponsePart::END) {
        runCommand(cnv, action);
        return dialogue->getPronoun() + " turns away!\n";
    }
    else if (action == ResponsePart::ATTACK) {
        runCommand(cnv, action);
        return string("\n") + getName() + " says: On guard! Fool!";
    }

    if (npcType == NPC_TALKER_BEGGAR && strncasecmp(inquiry, "give", 4) == 0) {
        reply.erase();
        cnv->state = Conversation::GIVEBEGGAR;
    }

    else if (strncasecmp(inquiry, "join", 4) == 0 &&
             c->party->canPersonJoin(getName(), &v)) {
        CannotJoinError join = c->party->join(getName());

        if (join == JOIN_SUCCEEDED) {
            reply += "I am honored to join thee!";
            c->location->map->removeObject(this);
            cnv->state = Conversation::DONE;
        } else {
            reply += "Thou art not ";
            reply += (join == JOIN_NOT_VIRTUOUS) ? getVirtueAdjective(v) : "experienced";
            reply += " enough for me to join thee.";
        }
    }

    else if ((*dialogue)[inquiry]) {        
        Dialogue::Keyword *kw = (*dialogue)[inquiry];

        reply = processResponse(cnv, kw->getResponse());
    }

    else if (settings.debug && strncasecmp(inquiry, "dump", 4) == 0) {
        vector<string> words = split(inquiry, " \t");
        if (words.size() <= 1)
            reply = dialogue->dump("");
        else
            reply = dialogue->dump(words[1]);
    }

    else
        reply += processResponse(cnv, dialogue->getDefaultAnswer());

    return reply;
}

string Person::talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
    bool valid = false;
    bool yes;
    char ans = tolower(answer[0]);

    if (ans == 'y' || ans == 'n') {
        valid = true;
        yes = ans == 'y';
    }

    if (!valid) {
        cnv->state = Conversation::ASKYESNO;
        return "Yes or no!";
    } 

    cnv->state = Conversation::TALK;
    return "\n" + processResponse(cnv, cnv->question->getResponse(yes));
}

string Person::beggarGetQuantityResponse(Conversation *cnv, const char *response) {
    string reply;

    cnv->quant = (int) strtol(response, NULL, 10);
    cnv->state = Conversation::TALK;

    if (cnv->quant > 0) {
        if (c->party->donate(cnv->quant)) {
            reply = "\n";
            reply += dialogue->getPronoun();
            reply += " says: Oh Thank thee! I shall never forget thy kindness!\n";
        }

        else
            reply = "\n\nThou hast not that much gold!\n";
    } else
        reply = "\n";

    return reply;
}

string Person::lordBritishGetQuestionResponse(Conversation *cnv, const char *answer) {
    string reply;

    cnv->state = Conversation::TALK;

    if (tolower(answer[0]) == 'y') {
        reply = "Y\n\nHe says: That is good.\n";
    }

    else if (tolower(answer[0]) == 'n') {
        reply = "N\n\nHe says: Let me heal thy wounds!\n";
        cnv->state = Conversation::FULLHEAL;           
    }

    else
        reply = "\n\nThat I cannot\nhelp thee with.\n";

    return reply;
}

string Person::getQuestion(Conversation *cnv) {
    return "\n" + cnv->question->getText() + "\n\nYou say: ";
}

/**
 * Returns the number of characters needed to get to 
 * the next line of text (based on column width).
 */
int chars_to_next_line(const char *s, int columnmax) {
    int chars = -1;

    if (strlen(s) > 0) {
        int lastbreak = columnmax;
        chars = 0;
        for (const char *str = s; *str; str++) {
            if (*str == '\n')
                return (str - s);
            else if (*str == ' ')
                lastbreak = (str - s);
            else if (++chars >= columnmax)
                return lastbreak;
        }
    }

    return chars;
}

/**
 * Counts the number of lines (of the maximum width given by
 * columnmax) in the string.
 */
int linecount(const string &s, int columnmax) {
    int lines = 0;
    unsigned ch = 0;
    while (ch < s.length()) {
        ch += chars_to_next_line(s.c_str() + ch, columnmax);
        if (ch < s.length())
            ch++;
        lines++;
    }
    return lines;
}

/**
 * Returns the number of characters needed to produce a
 * valid screen of text (given a column width and row height)
 */
int chars_needed(const char *s, int columnmax, int linesdesired, int *real_lines) {
    int chars = 0,
        totalChars = 0;    

    char *new_str = strdup(s),
         *str = new_str;    

    // try breaking text into paragraphs first
    string text = s;
    string paragraphs;
    unsigned int pos;
    int lines = 0;
    while ((pos = text.find("\n\n")) < text.length()) {
        string p = text.substr(0, pos);
        lines += linecount(p.c_str(), columnmax);
        if (lines <= linesdesired)
            paragraphs += p + "\n";
        else break;
        text = text.substr(pos+1);
    }
    // Seems to be some sort of clang compilation bug in this code, that causes this addition
    // to not work correctly.
    int totalPossibleLines = lines + linecount(text.c_str(), columnmax);
    if (totalPossibleLines <= linesdesired)
        paragraphs += text;

    if (!paragraphs.empty()) {
        *real_lines = lines;
        free(new_str);
        return paragraphs.length();
    }
    else {
        // reset variables and try another way
        lines = 1;
    }
    // gather all the line breaks
    while ((chars = chars_to_next_line(str, columnmax)) >= 0) {
        if (++lines >= linesdesired)
            break;

        int num_to_move = chars;
        if (*(str + num_to_move) == '\n')
            num_to_move++;
        
        totalChars += num_to_move;
        str += num_to_move;
    }

    free(new_str);

    *real_lines = lines;
    return totalChars;    
}
