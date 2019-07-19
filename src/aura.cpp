/*
 * $Id: aura.cpp 3019 2012-03-18 11:31:13Z daniel_santos $
 */

#include "vc6.h"

#include "aura.h"

Aura::Aura() : type(NONE), duration(0) {}

void Aura::setDuration(int d) {
    duration = d;
    setChanged();
    notifyObservers(NULL);
}

void Aura::set(Type t, int d) {
    type = t;
    duration = d;
    setChanged();
    notifyObservers(NULL);
}

void Aura::setType(Type t) {
    type = t;
    setChanged();
    notifyObservers(NULL);
}

void Aura::passTurn() {
    if (duration > 0) {
        duration--;
        
        if (duration == 0) {
            type = NONE;

            setChanged();
            notifyObservers(NULL);
        }
    }
}
