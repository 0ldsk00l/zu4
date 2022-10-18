#ifndef AURA_H
#define AURA_H

#include "observable.h"

typedef enum _AuraType {
	AURA_NONE,
	AURA_HORN,
	AURA_JINX,
	AURA_NEGATE,
	AURA_PROTECTION,
	AURA_QUICKNESS
} AuraType;

struct Aura : public Observable<Aura *> {
public:
    Aura();

    int getDuration() const         { return duration; }
    bool isActive() const           { return duration > 0; }

    void setDuration(int d);
    void set(AuraType = AURA_NONE, int d = 0);
    void setType(AuraType t);

    void passTurn();

    AuraType type;
    int duration;
};

#endif
