/*
 * $Id: u4_sdl.cpp 3019 2012-03-18 11:31:13Z daniel_santos $
 */



#include <SDL.h>
#include "u4_sdl.h"

static inline int u4_SDL_Init() {
    return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
}

static inline void u4_SDL_Delete() {
    SDL_Quit();
}

int u4_SDL_InitSubSystem(Uint32 flags) {
    int f = SDL_WasInit(SDL_INIT_EVERYTHING);
    if (f == 0) {
        u4_SDL_Init();
    }
    if (!SDL_WasInit(flags))
        return SDL_InitSubSystem(flags);
    else return 0;    
}

void u4_SDL_QuitSubSystem(Uint32 flags) {
    if (SDL_WasInit(SDL_INIT_EVERYTHING) == flags)
        u4_SDL_Delete();
    else SDL_QuitSubSystem(flags);
}
