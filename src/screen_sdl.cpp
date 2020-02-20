/*
 * $Id: screen_sdl.cpp 3087 2015-01-24 03:44:46Z darren_janeczek $
 */

#include <SDL.h>
#include <GL/glu.h>

#include "error.h"
#include "image.h"
#include "settings.h"
#include "u4_sdl.h"

static GLuint texID = 0;

static void ogl_init() {
	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glViewport(0, 0, SCREEN_WIDTH * settings.scale, SCREEN_HEIGHT * settings.scale);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_3D_EXT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, SCREEN_WIDTH * settings.scale, SCREEN_HEIGHT * settings.scale, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void ogl_swap() {
	Image *screen = xu4_img_get_screen();
	glTexImage2D(GL_TEXTURE_2D,
				0,
				GL_RGB,
				SCREEN_WIDTH, SCREEN_HEIGHT,
				0,
				GL_RGBA,
				GL_UNSIGNED_BYTE,
		screen->pixels);
	
	glBegin(GL_QUADS);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(SCREEN_WIDTH * settings.scale, SCREEN_HEIGHT * settings.scale);
		
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(SCREEN_WIDTH * settings.scale, 0.0);
		
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0, 0.0);
		
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(0, SCREEN_HEIGHT * settings.scale);
	glEnd();
	SDL_GL_SwapBuffers();
}

void screenInit_sys() {
    if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        xu4_error(XU4_LOG_ERR, "Unable to init SDL: %s", SDL_GetError());
	}
    
    SDL_EnableUNICODE(1);
    SDL_SetGamma(settings.gamma / 100.0f, settings.gamma / 100.0f, settings.gamma / 100.0f);
    atexit(SDL_Quit);

    SDL_WM_SetCaption("Ultima IV", NULL);
    
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    if (!SDL_SetVideoMode(SCREEN_WIDTH * settings.scale, SCREEN_HEIGHT * settings.scale, 16, SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_OPENGL))
        xu4_error(XU4_LOG_ERR, "unable to set video: %s", SDL_GetError());

    SDL_ShowCursor(SDL_DISABLE);
    ogl_init();
}

void screenDelete_sys() {
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void screenRedrawScreen() {
    ogl_swap();
}
