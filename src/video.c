/*
 * video.c
 * Copyright (C) 2020 R. Danbrook
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include <SDL.h>
#include <GL/glu.h>

#include "error.h"
#include "image.h"
#include "settings.h"
#include "u4_sdl.h"

static SDL_Window *window;
static SDL_GLContext glcontext;

static GLuint texID = 0;

static void xu4_ogl_init() {
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

void xu4_ogl_swap() {
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
	SDL_GL_SwapWindow(window);
}

void xu4_video_init() {
    if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        xu4_error(XU4_LOG_ERR, "Unable to init SDL: %s", SDL_GetError());
	}
	
    atexit(SDL_Quit);
    
	SDL_ShowCursor(SDL_DISABLE);
	
	window = SDL_CreateWindow("Ultima IV",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH * settings.scale, SCREEN_HEIGHT * settings.scale,
		SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL);
	
	glcontext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glcontext);
	SDL_GL_SetSwapInterval(1);
	xu4_ogl_init();
}

void xu4_video_deinit() {
    SDL_DestroyWindow(window);
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);
    if (texID) { glDeleteTextures(1, &texID); }
}
