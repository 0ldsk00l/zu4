/*
 * $Id: dialogueloader_tlk.h 2104 2004-08-23 19:44:00Z dougday $
 */

#ifndef DIALOGUELOADER_TLK_H
#define DIALOGUELOADER_TLK_H

#include "dialogueloader.h"

/**
 * The dialogue loader for u4dos .tlk files
 */
class U4TlkDialogueLoader : public DialogueLoader {
    static DialogueLoader *instance;

public:
    virtual Dialogue *load(void *source);
};

#endif
