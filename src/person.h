/*
 * $Id: person.h 2663 2006-01-23 06:57:55Z andrewtaylor $
 */

#ifndef PERSON_H
#define PERSON_H

#include <list>
#include <string>

#include "creature.h"
#include "types.h"

struct Conversation;
struct Dialogue;
struct Response;
struct ResponsePart;

typedef enum {
   NPC_EMPTY,
   NPC_TALKER,
   NPC_TALKER_BEGGAR,
   NPC_TALKER_GUARD,
   NPC_TALKER_COMPANION,
   NPC_VENDOR_WEAPONS,
   NPC_VENDOR_ARMOR,
   NPC_VENDOR_FOOD,
   NPC_VENDOR_TAVERN,
   NPC_VENDOR_REAGENTS,
   NPC_VENDOR_HEALER,
   NPC_VENDOR_INN,
   NPC_VENDOR_GUILD,
   NPC_VENDOR_STABLE,
   NPC_LORD_BRITISH,
   NPC_HAWKWIND,
   NPC_MAX
} PersonNpcType;

struct Person : public Creature {
public:
    Person(MapTile tile);
    Person(const Person *p);

    bool canConverse() const;
    bool isVendor() const;
    virtual std::string getName() const;
    void goToStartLocation();
    void setDialogue(Dialogue *d);
    Coords &getStart() { return start; }
    PersonNpcType getNpcType() const { return npcType; }
    void setNpcType(PersonNpcType t);

    std::list<std::string> getConversationText(Conversation *cnv, const char *inquiry);
    std::string getPrompt(Conversation *cnv);
    const char *getChoices(Conversation *cnv);

    std::string getIntro(Conversation *cnv);
    std::string processResponse(Conversation *cnv, Response *response);
    void runCommand(Conversation *cnv, const ResponsePart &command);
    std::string getResponse(Conversation *cnv, const char *inquiry);
    std::string talkerGetQuestionResponse(Conversation *cnv, const char *inquiry);
    std::string beggarGetQuantityResponse(Conversation *cnv, const char *response);
    std::string lordBritishGetQuestionResponse(Conversation *cnv, const char *answer);
    std::string getQuestion(Conversation *cnv);

private:
    Dialogue* dialogue;
    Coords start;
    PersonNpcType npcType;
};

bool isPerson(Object *punknown);

std::list<std::string> replySplit(const std::string &text);
int linecount(const std::string &s, int columnmax);

#endif
