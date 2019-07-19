/*
 * $Id: dialogueloader_hw.h 2663 2006-01-23 06:57:55Z andrewtaylor $
 */

#ifndef DIALOGUELOADER_HW_H
#define DIALOGUELOADER_HW_H

#include "dialogueloader.h"

/**
 * The dialogue loader for Hawkwind.
 */
class U4HWDialogueLoader : public DialogueLoader {
    static DialogueLoader *instance;

public:
    virtual Dialogue *load(void *source);
};

#endif
