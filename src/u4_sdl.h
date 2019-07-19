/*
 * $Id: u4_sdl.h 2806 2011-01-28 04:35:31Z darren_janeczek $
 */

#ifndef U4_SDL_H
#define U4_SDL_H

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif


int u4_SDL_InitSubSystem(Uint32 flags);
void u4_SDL_QuitSubSystem(Uint32 flags);

#ifdef __cplusplus
}
#endif

#endif
