/*
 * $Id: shrine.h 2171 2004-09-08 05:07:18Z andrewtaylor $
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "map.h"
#include "savegame.h"

#define SHRINE_MEDITATION_INTERVAL  100
#define MEDITATION_MANTRAS_PER_CYCLE 16

/*typedef struct _Shrine {
    Virtue virtue;
    const char *mantra;
} Shrine;*/

struct Shrine : public Map {
public:
    Shrine();

    // Methods
    virtual std::string getName();
    Virtue          getVirtue() const;
    std::string     getMantra() const;

    void            setVirtue(Virtue v);
    void            setMantra(std::string mantra);

    void enter();
    void enhancedSequence();
    void meditationCycle();
    void askMantra();
    void eject();
    void showVision(bool elevated);

    // Properties
private:
    std::string name;
    Virtue virtue;
    std::string mantra;   
};

bool shrineCanEnter(const struct _Portal *p);
bool isShrine(Map *punknown);

#endif
