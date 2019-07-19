/*
 * $Id: dialogueloader_lb.h 2622 2005-11-16 07:41:22Z andrewtaylor $
 */

#ifndef DIALOGUELOADER_LB_H
#define DIALOGUELOADER_LB_H

#include "dialogueloader.h"

/**
 * The dialogue loader for Lord British
 */
class U4LBDialogueLoader : public DialogueLoader {
    static DialogueLoader *instance;

public:
    virtual Dialogue *load(void *source);
};

#endif
