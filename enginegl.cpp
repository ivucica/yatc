//////////////////////////////////////////////////////////////////////
// Yet Another Tibia Client
//////////////////////////////////////////////////////////////////////
// OpenGL engine
//////////////////////////////////////////////////////////////////////
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////

/// \file enginegl.cpp
///
/// Contains the code for OpenGL rendering engine.
///
/// \sa EngineGL

#ifdef USE_OPENGL
#include <SDL/SDL_opengl.h>

#include <GLICT/globals.h>
#include <time.h>
#include "options.h"
#include "enginegl.h"
#include "font.h"
EngineGL::EngineGL()
{
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Starting OpenGL engine\n");
	m_screen = NULL;

    SDL_ShowCursor(0);
	doResize(m_width, m_height);

	if(!m_screen)
		// looks like GL is not supported -- dont attempt anything else
		return;

	glictGlobals.drawPartialOut = true;
	glictGlobals.clippingMode = GLICT_SCISSORTEST;

    Sprite*a,*b;

	m_sysfont->SetFontParam(new YATCFont("Tibia.pic", 2, a=createSprite("Tibia.pic", 2)));
	m_minifont->SetFontParam(new YATCFont("Tibia.pic", 5, createSprite("Tibia.pic", 5)));
	m_aafont->SetFontParam(new YATCFont("Tibia.pic", 7, b=createSprite("Tibia.pic", 7)));
	m_gamefont->SetFontParam(new YATCFont("Tibia.pic", 4, createSprite("Tibia.pic", 4)));

	a->addColor(.75,.75,.75);
	b->addColor(.75,.75,.75);

    m_ui = createSprite("Tibia.pic", 3);
	m_light = createSprite("Tibia.pic", 6);
    m_cursorBasic = m_ui->createCursor(290,12,11,19, 1, 1);
    m_cursorUse = m_ui->createCursor(310,12,19,19, 9, 9);
    SDL_ShowCursor(1);
    SDL_SetCursor(m_cursorBasic);

	initEngine();
}

EngineGL::~EngineGL()
{
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Closing OpenGL engine\n");
	if (m_screen) {
		delete (YATCFont*)m_sysfont->GetFontParam();
		delete (YATCFont*)m_minifont->GetFontParam();
		delete (YATCFont*)m_aafont->GetFontParam();
		delete (YATCFont*)m_gamefont->GetFontParam();
	}
}

void EngineGL::initEngine()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void EngineGL::doResize(int& w, int& h)
{
	Engine::doResize(w, h);

	if (m_screen) SDL_FreeSurface(m_screen);
	m_videoflags = SDL_OPENGL | SDL_RESIZABLE | SDL_DOUBLEBUF | SDL_HWSURFACE;

	m_creationTimestamp = time(NULL);

	if (options.fullscreen)
		m_videoflags |= SDL_FULLSCREEN;

	m_screen = SDL_SetVideoMode(m_width, m_height, m_video_bpp, m_videoflags);

	if(!m_screen){
		fprintf(stderr, "Could not set %dx%d video mode: %s\n", m_width, m_height, SDL_GetError());
		return;
	}

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void EngineGL::drawLightmap(vertex* lightmap, int type, int width, int height, float scale)
{
	if (type == 3){
		glDisable(GL_TEXTURE_2D);

                glBlendFunc(GL_DST_COLOR, GL_ZERO);

		for (int y = 0; y < height - 1; ++y){
			glBegin(GL_TRIANGLE_STRIP);
			for (int x = 0; x < width; ++x){
				glColor4f((lightmap[(((y + 1) * width) + x)].r / lightmap[(((y + 1) * width) + x)].blended) / 255.0f, (lightmap[(((y + 1) * width) + x)].g / lightmap[(((y + 1) * width) + x)].blended) / 255.0f, (lightmap[(((y + 1) * width) + x)].b / lightmap[(((y + 1) * width) + x)].blended) / 255.0f, lightmap[(((y + 1) * width) + x)].alpha / 255.0f);
				glVertex2f(lightmap[(((y + 1) * width) + x)].x, lightmap[(((y + 1) * width) + x)].y);

				glColor4f((lightmap[((y * width) + x)].r / lightmap[((y * width) + x)].blended) / 255.0f, (lightmap[((y * width) + x)].g / lightmap[((y * width) + x)].blended) / 255.0f, (lightmap[((y * width) + x)].b / lightmap[((y * width) + x)].blended) / 255.0f, lightmap[((y * width) + x)].alpha / 255.0f);
				glVertex2f(lightmap[((y * width) + x)].x, lightmap[((y * width) + x)].y);
			}
			glEnd();
		}

                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else{
		Engine::drawLightmap(lightmap, type, width, height, scale);
	}
}

void EngineGL::drawRectangle(float x, float y, float width, float height, oRGBA color)
{
	glDisable(GL_TEXTURE_2D);

	glColor4f(color.r/255.0f, color.g/255.0f, color.b/255.0f, color.a/255.0f);

	glRectf(x, y, x+width, y+height);

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void EngineGL::drawRectangleLines(float x, float y, float width, float height, oRGBA color, float thickness /*= 1.f*/)
{
    glDisable(GL_TEXTURE_2D);

	glColor4f(color.r/255.0f, color.g/255.0f, color.b/255.0f, color.a/255.0f);
	//TODO (nfries88): implement thickness
	glBegin(GL_LINE_LOOP);
	glVertex2f(x,y);
	glVertex2f(x+width,y);
	glVertex2f(x+width,y+height);
	glVertex2f(x,y+height);
	glEnd();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

bool EngineGL::isSupported()
{
	if (!m_screen)
		return false;
	else
		return true;
	/*uint32_t vf = SDL_OPENGL | SDL_RESIZABLE;

	SDL_Surface *s = SDL_SetVideoMode(m_width, m_height, m_video_bpp, m_videoflags);
	if (s) {
		SDL_FreeSurface(s);
		return true;
	} else
		return false;*/
}
#endif
