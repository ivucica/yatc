//////////////////////////////////////////////////////////////////////
// Yet Another Tibia Client
//////////////////////////////////////////////////////////////////////
//
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

#ifndef _MSC_VER
#include <stdint.h>
#endif
#include "gm_gameworld.h"
#include "popup.h"
#include "gamecontent/globalvars.h"
#include "gamecontent/creature.h"
#include "gamecontent/item.h"
#include "mapui.h"
#include "debugprint.h"
#include "engine.h"
#include "net/protocolgame.h"
#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif
#if defined(HAVE_LIBINTL_H)
	#include <libintl.h>
#else
	#define gettext(x) (x)
#endif

extern uint32_t g_frameTime;
extern Engine* g_engine;

#include "options.h"
#include "gamecontent/creature.h"

inline oRGBA makeLightColor(uint8_t lightColor){
    // NOTE (nfries88): this works for colors sent from the server, but colors from .dat are mysteriously 16-bit rather than 8-bit... who knows
        // also, ripped this from minimap code. Surprisingly, works.
    oRGBA color;
    color.b = uint8_t((lightColor % 6) / 5. * 255);
	color.g = uint8_t(((lightColor / 6) % 6) / 5. * 255);
	color.r = uint8_t((lightColor / 36.) / 6. * 255);
    return color;
}

MapUI::MapUI()
{
	m_x = 0;
	m_y = 0;

	// tilecount: width and height to render  -- "viewport" width and height
	#ifndef WINCE
	m_vpw=18, m_vph=14;
	#else
	m_vpw=8, m_vph=6;
	#endif


	m_w = m_vpw*32; // just defaults
	m_h = m_vph*32;

    m_scale = 1.F;

    m_minz = -1;

    lightmap = new vertex[m_vpw * m_vph];
}

MapUI::~MapUI()
{
    delete [] lightmap;
}

void MapUI::fillLightCircle(int x, int y, int radius, uint16_t color)
{
	if (radius > 0)
	{
		oRGBA realColor = makeLightColor(color);
		for (int i = 0; i < m_vpw; ++i)
		{
			for (int j = 0; j < m_vph; ++j)
			{
				float dist = std::max((float)sqrt(pow((double)(i - x), (double)2) + pow((double)(j - y), (double)2)), 0.0f);
				if (i >= 0 && j >= 0 && i < m_vpw && j < m_vph)
				{
				    int index = ((j * m_vpw) + i);
					float influence = dist / radius;
					lightmap[index].alpha = std::min((int)(lightmap[index].alpha), 255 - (int)std::max((int)(255 * (1 - (influence))), (int)(0)));

					if (dist <= radius)
					{
						lightmap[index].r += realColor.r - (realColor.r * influence);
						lightmap[index].g += realColor.g - (realColor.g * influence);
						lightmap[index].b += realColor.b - (realColor.b * influence);
						lightmap[index].alpha += realColor.a - (realColor.a * influence);
						++lightmap[index].blended;
					}
				}
			}
		}
	}
}

bool MapUI::isVisible(const Position& pos)
{
    // you're one level underground or on the topmost floor; you won't be covered.
    if(pos.z == 6 || pos.z == 0) return true;
    if(m_minz < 0) getMinZ();
    int maxz = m_minz;
    int minz = pos.z-1;

/*for(z = maxz; z <= minz; z++){
		tile = Map::getInstance().getTile(pos.x + x - (z-pos.z) - m_vpw/2, pos.y + y - (z-pos.z) - m_vph/2, z);
		if(tile){ // found you
			break;
		}
	}*/

    for(int z = maxz; z <= minz; z++){
        int offset = z-pos.z;
        Tile* tile = Map::getInstance().getTile(pos.x - offset, pos.y - offset, z);
        if(tile) {
            if(tile->getGround())// && !tile->getGround()->isSeeThrough())
                return false;
        }
    }
    return true;
}

void MapUI::renderMap()
{
	//TODO: center game area horizontally (cipsoft's client does this)

	// set up scale
	if(!options.stretchGameWindow)
		GlobalVariables::setScale(MIN(m_w/(15.*32), m_h/(11.*32)));
		//m_scale = MIN(m_w/(15.*32), m_h/(11.*32));
	else
        GlobalVariables::setScale(1.f);
		//m_scale = 1.f;
	// NOTE (nfries88): By flooring scaledSize, we assure a full pixel value for scaledSize.
	//	by then setting m_scale to this full pixel value / 32, we assure that all things are scaled equally.

	// NOTE (Kilouco): Made Scale var global to be used everywhere without some annoying restrictions.
	m_scale = GlobalVariables::getScale();
	float scaledSize = std::floor(32*m_scale);
	GlobalVariables::setScale(scaledSize/32);
	//m_scale = scaledSize/32;
	m_scale = GlobalVariables::getScale();

    m_x = int(-scaledSize*2); m_y = int(-scaledSize*2);
	g_engine->setClipping(/*scaledSize*2 + m_x,scaledSize*2 + m_y,*/0,0,int(15*scaledSize),int(11*scaledSize));

	// NOTE (nfries88): Draw black under the game area, this will make blank tiles appear black like in the official client.
	g_engine->drawRectangle(0, 0, 15*scaledSize, 11*scaledSize, oRGBA(0, 0, 0, 255));

	// get player position
	Position pos = GlobalVariables::getPlayerPosition();

	// find out how far above the player shall be visible
	if(m_minz < 0) getMinZ();
	int m = m_minz;
	int sz;
	if(pos.z > 7){ // underground
		sz = MIN(pos.z + 3, 15);
	}
	else{
		sz = 7;
	}

	// find out scrolling offset
	float walkoffx = 0.f, walkoffy = 0.f;
	Creature* player = Creatures::getInstance().getPlayer();
	if(player){
		player->getWalkOffset(walkoffx, walkoffy, m_scale);
	}
	walkoffx *= -1; walkoffy *= -1;

    // reset light map
    memset((void*)lightmap, 0, sizeof(vertex) * m_vpw * m_vph);
    float ambientBrightness = 1.0 - GlobalVariables::getWorldLightLevelScaled();
    if (pos.z <= 7) {
      ambientBrightness = 0;
    }

    oRGBA initColor = (pos.z <= 7 ? (makeLightColor(GlobalVariables::getWorldLightColor())) : oRGBA(0, 0, 0, 255));
    //oRGBA initColor = oRGBA(0, 0, 0, 255);
	Tile::EffectList::iterator effectIt;

    // for debugging purposes
    for (int i = 0; i < m_vpw; ++i)
    {
        for (int j = 0; j < m_vph; j++)
        {
          // The lower the ambientBrightness, the darker the base color is.
          // The higher the ambientBrightness, the more tinted is the base environment using the base color. (e.g. with 216 = red, at 255, it will be very red.)
          // Later, lamps can color it in other colors.

          // Without addressing how the lights are rendered, we can't easily do this in SDL.
          // We can simulate this to an extent by lowering the alpha to 128.0 during full daylight: at 255, we'd create an impenetrable fog-of-war
          // during daytime and not just during the night.

          lightmap[(j * m_vpw) + i].alpha = ambientBrightness; // 128.0 + (128.0 * ambientBrightness);
          // lightmap[(j * m_vpw) + i].alpha = /*initColor.a * ambientBrightness*/ 255;
			lightmap[(j * m_vpw) + i].blended = 1;
			lightmap[(j * m_vpw) + i].r = initColor.r * (1-ambientBrightness);
			lightmap[(j * m_vpw) + i].g = initColor.g * (1-ambientBrightness);
			lightmap[(j * m_vpw) + i].b = initColor.b * (1-ambientBrightness);
        }
    }

	for(int z = sz; z >= m; z--)
	{

		ASSERT(z >= 0);
		int offset = z - pos.z;

		for(uint32_t i = 0; i < m_vpw; ++i){
			for(uint32_t j = 0; j < m_vph; ++j){

				uint32_t tile_height = 0;

				uint32_t tile_x = pos.x + i - m_vpw/2 - offset;
				uint32_t tile_y = pos.y + j - m_vph/2 - offset;

				Tile* tile = Map::getInstance().getTile(tile_x, tile_y, z);

				if(!tile){
					continue;
				}

				// Add light for any effects on this tile.
				Tile::EffectList& tileEffects = const_cast<Tile*>(tile)->getEffects();
				effectIt = tileEffects.begin();
				while(effectIt != tileEffects.end()){
					fillLightCircle(i-offset, j-offset, (*effectIt)->getObjectType()->lightLevel, (*effectIt)->getObjectType()->lightColor);
					++effectIt;
				}

				int screenx = int((i*scaledSize + walkoffx) + m_x);
				int screeny = int((j*scaledSize + walkoffy) + m_y);

				int index    = ((j * m_vpw) + i);
                lightmap[index].x = screenx;
                lightmap[index].y = screeny;

				const Item* ground = tile->getGround();
				if(ground){
					ground->Blit(screenx, screeny, m_scale, tile_x, tile_y);

					if(ground->hasHeight())
						tile_height++;

					//fillLightCircle(i, j, ground->getObjectType()->lightLevel, makeLightColor(ground->getObjectType()->lightColor));
					fillLightCircle(i-offset, j-offset, ground->getObjectType()->lightLevel, ground->getObjectType()->lightColor);
				}

				enum drawingStates_t{
					DRAW_ITEMTOP1 = 1,
					DRAW_ITEMTOP2,
					DRAW_ITEMDOWN,
					DRAW_CREATURE,
					DRAW_ITEMTOP3
				};
				drawingStates_t drawState = DRAW_ITEMTOP1;

				int32_t thingsCount = tile->getThingCount() - 1;
				int32_t drawIndex = ground ? 1 : 0, lastTopIndex = 0;
				bool drawnEffectsGhosts = false;

				while(drawIndex <= thingsCount && drawIndex >= 0){

					const Thing* thing = tile->getThingByStackPos(drawIndex);
					if(thing){
						int32_t thingOrder = thing->getOrder();

						switch(drawState){
						case DRAW_ITEMTOP1: //topItems 1,2
						case DRAW_ITEMTOP2:
							if(thingOrder > 2){
								drawState = DRAW_ITEMDOWN;
								lastTopIndex = drawIndex;
								drawIndex = thingsCount;
								continue;
							}
							break;

						case DRAW_ITEMDOWN: //downItems
							if(thingOrder != 5){
								drawState = DRAW_CREATURE;
								continue;
							}
							break;

						case DRAW_CREATURE: //creatures
							if(thingOrder != 4){
							    //Draw ghosting and effects
							    drawTileGhosts(tile_x, tile_y, z, screenx, screeny, m_scale, tile_height);
								drawTileEffects(const_cast<Tile*>(tile), screenx, screeny, m_scale, tile_height);
								drawnEffectsGhosts = true;
								//
								drawState = DRAW_ITEMTOP3;
								drawIndex = lastTopIndex;
								continue;
							}
							break;

						case DRAW_ITEMTOP3: //topItems 3
							if(thingOrder != 3){
								drawIndex = -1; //force exit loop
								continue;
							}
							break;
						}

                        bool performPaint = true;
                        if(const Creature* c = thing->getCreature()){
                            if(c->getWalkState() != 1. || c->isPreWalking()){// it's walking?
                                if (!((c->getLookDir() == DIRECTION_SOUTH && !c->isPreWalking()) ||
                                    (c->getLookDir() == DIRECTION_NORTH && c->isPreWalking()) ||
                                    (c->getLookDir() == DIRECTION_EAST && !c->isPreWalking()) ||
                                    (c->getLookDir() == DIRECTION_WEST && c->isPreWalking())))
                                    performPaint = false;
                            }
                            //fillLightCircle(i, j, c->getLightLevel(), makeLightColor(c->getLightColor()));
                            if(isVisible(tile->getPos()))
                                fillLightCircle(i-offset, j-offset, c->getLightLevel(), c->getLightColor());
                        }

						if (performPaint)
						    thing->Blit(screenx - (int)(tile_height*8.*m_scale),
							    		screeny - (int)(tile_height*8.*m_scale),
								    	m_scale, tile_x, tile_y);

						if(const Item* item = thing->getItem()){
							if(item->hasHeight())
								tile_height++;

							//fillLightCircle(i, j, item->getObjectType()->lightLevel, makeLightColor(item->getObjectType()->lightColor));
							if(isVisible(tile->getPos()))
                                fillLightCircle(i-offset, j-offset, item->getObjectType()->lightLevel, item->getObjectType()->lightColor);
						}

						switch(drawState){
						case DRAW_ITEMTOP1: //topItems 1,2
						case DRAW_ITEMTOP2:
							drawIndex++;
							break;
						case DRAW_ITEMDOWN: //downItems
						case DRAW_CREATURE: //creatures
							drawIndex--;
							break;
						case DRAW_ITEMTOP3: //topItems 3
							drawIndex++;
							break;
						}

					}
					else{
						DEBUGPRINT(DEBUGPRINT_WARNING, DEBUGPRINT_LEVEL_OBLIGATORY, "Thing invalid %d\n", drawIndex);
						break;
					}
				}
				if(!drawnEffectsGhosts){
				    drawTileGhosts(tile_x, tile_y, z, screenx, screeny, m_scale, tile_height);
					drawTileEffects(const_cast<Tile*>(tile), screenx, screeny, m_scale, tile_height);
				}
			}
		}

		//draw animated texts
		if (options.showtexteffects) {
            Map::AnimatedTextList& aniTexts = Map::getInstance().getAnimatedTexts(z);
            Map::AnimatedTextList::iterator it = aniTexts.begin();
            while(it != aniTexts.end()){
                if((*it).canBeDeleted()){
                    aniTexts.erase(it++);
                }
                else{
                    const Position& txtpos = (*it).getPosition();

                    float textYOffset = (g_frameTime - (*it).getStartTime())/1000.f*0.75f;

                    int screenx = (int)((txtpos.x - pos.x + m_vpw/2 + offset + 0.4)*scaledSize + walkoffx) + m_x;
                    int screeny = (int)((txtpos.y - pos.y + m_vph/2 + offset - textYOffset)*scaledSize + walkoffy) + m_y;

                    g_engine->drawTextGW((*it).getText().c_str() , "gamefont", screenx, screeny-(glictFontNumberOfLines((*it).getText().c_str())*12), m_scale, (*it).getColor());
                    ++it;
                }
            }
        }

		//draw distance effects
		{
		Map::DistanceEffectList& distanceEffects = Map::getInstance().getDistanceEffect(z);
		Map::DistanceEffectList::iterator it = distanceEffects.begin();

		// TODO: Distance effects need light too!
        if (distanceEffects.size()) printf("Drawing %d distance shots\n", distanceEffects.size());
		while(it != distanceEffects.end()){
			float flightProgress = (*it)->getFlightProgress();

			if(flightProgress > 1.f){
				delete *it;
				distanceEffects.erase(it++);
				printf("Removed a distance shot\n");
			}
			else{
				const Position& fromPos = (*it)->getFromPos();
				const Position& toPos = (*it)->getToPos();

				float screenxFrom = ((fromPos.x - pos.x + m_vpw/2 + offset)*scaledSize + walkoffx) + m_x;
				float screenyFrom = ((fromPos.y - pos.y + m_vph/2 + offset)*scaledSize + walkoffy) + m_y;

				float screenxTo = ((toPos.x - pos.x + m_vpw/2 + offset)*scaledSize + walkoffx) + m_x;
				float screenyTo = ((toPos.y - pos.y + m_vph/2 + offset)*scaledSize + walkoffy) + m_y;

				int screenx = (int)(screenxFrom + (screenxTo - screenxFrom)*flightProgress);
				int screeny = (int)(screenyFrom + (screenyTo - screenyFrom)*flightProgress);

				(*it)->Blit(screenx, screeny, m_scale);
				++it;
			}
		}
		}
	}

	// draw always-on-top things
	// (currently only creature names)

	int playerspeed = 0;
	for(uint32_t i = 0; i < m_vpw; ++i){
		for(uint32_t j = 0; j < m_vph; ++j){
			Tile* tile = Map::getInstance().getTile(pos.x + i - m_vpw/2, pos.y + j - m_vph/2, pos.z);
			if(!tile){
				continue;
			}

			int screenx = (int)(i*scaledSize + walkoffx) + m_x;
			int screeny = (int)(j*scaledSize + walkoffy) + m_y;
			int groundspeed = tile->getSpeedIndex();

			int32_t thingsCount = tile->getThingCount() - 1;
			int32_t drawIndex = 1;
			while(drawIndex <= thingsCount){

				Thing* thing = tile->getThingByStackPos(drawIndex);
				if(thing){
					if(thing->getCreature()){
                        thing->getCreature()->drawInfo(screenx, screeny, m_scale, isVisible(tile->getPos()));
						if(!player || thing->getCreature()->getId() != player->getId())
							thing->getCreature()->advanceWalk(groundspeed);
						else
							playerspeed = groundspeed;
					}
				}
				drawIndex++;
			}
		}
	}

	// draw publicly displayed messages
    drawPublicMessages(pos, walkoffx, walkoffy);
    // draw private messages
    drawPrivateMessages();

	if(options.showlighteffects){
	    if((player != NULL) && !(player->getLightLevel())){
	        fillLightCircle(m_vpw/2, m_vph/2, 2, GlobalVariables::getWorldLightColor());
	    }
        g_engine->drawLightmap(lightmap, options.showlighteffects, m_vpw, m_vph, m_scale);
	}

	g_engine->resetClipping();

	if(player)
		player->advanceWalk(playerspeed);
}

void MapUI::drawPublicMessages(Position pos, float walkoffx, float walkoffy)
{
    // OPTIMIZE ME.
    Map::PublicMessageList& messages = Map::getInstance().getPublicMessages();
    Map::PublicMessageList::iterator it = messages.begin(), temp_it = messages.begin();
    Map::PublicMessageList::reverse_iterator rit = messages.rbegin(), temp_rit = messages.rbegin(), temp_rit2 = messages.rbegin();
    std::string text;
    int x = 0, y = 0, orange_linecount = 0, linecount = 0, first = 0;

    while(it != messages.end())
        if((*it).canBeDeleted())
            it = messages.erase(it);
        else
            it++;

    // NOTE (kilouco): In this loop we firstly handle public messages. Then we get those
    // which are in the same tile, said by the same player.

    // First we'll handle orange messages (which has priority).
    while(rit != messages.rend()){
        if ((*rit).shouldShowName()) {
            rit++;
            continue;
        }

        temp_rit = rit;
        const Position& txtpos = (*rit).getPosition();
        x = ((txtpos.x - pos.x + m_vpw/2 - 2) * (m_scale * 32) + (m_scale * 16)) + walkoffx;
        y = ((txtpos.y - pos.y + m_vph/2 - 2) * (m_scale * 32)) + walkoffy;

        if((*rit).is_handled() == false) {
            linecount = (*rit).getLinecount();
            (*rit).set_handled(true);
            (*rit).set_relativePos(linecount);

            while(temp_rit != messages.rend()){
                if ((*temp_rit).shouldShowName()) {
                    temp_rit++;
                    continue;
                }

                if((*rit).getPosition() == (*temp_rit).getPosition() && (*temp_rit).getSender() == (*rit).getSender() && (*temp_rit).is_handled() == false) {
                    linecount += (*temp_rit).getLinecount();
                    (*temp_rit).set_relativePos(linecount);
                    (*temp_rit).set_handled(true);
                }
                temp_rit++;
            }
        }
        rit++;
    }

    // Then we handle yellow messages, with names and stuff.
    linecount = 0;
    rit = messages.rbegin();
    while(rit != messages.rend()){
        if (!(*rit).shouldShowName()) {
            rit++;
            continue;
        }

        temp_rit = rit;
        temp_rit2 = messages.rbegin();
        const Position& txtpos = (*rit).getPosition();
        x = ((txtpos.x - pos.x + m_vpw/2 - 2) * (m_scale * 32) + (m_scale * 16)) + walkoffx;
        y = ((txtpos.y - pos.y + m_vph/2 - 2) * (m_scale * 32)) + walkoffy;

        if((*rit).is_handled() == false) {
            linecount = (*rit).getLinecount();
            orange_linecount = 0;

            (*rit).set_handled(true);

            while(temp_rit2 != messages.rend()) {
                if ((*temp_rit2).shouldShowName()) {
                    temp_rit2++;
                    continue;
                }

                if((*rit).getPosition() == (*temp_rit2).getPosition() && (*rit).getSender() == (*temp_rit2).getSender())
                    orange_linecount += (*temp_rit2).getLinecount();

                temp_rit2++;
            }

            (*rit).set_relativePos(orange_linecount + linecount);

            while(temp_rit != messages.rend()){
                if (!(*temp_rit).shouldShowName()) {
                    temp_rit++;
                    continue;
                }

                if((*rit).getPosition() == (*temp_rit).getPosition() && (*temp_rit).getSender() == (*rit).getSender() && (*temp_rit).is_handled() == false) {
                    linecount += (*temp_rit).getLinecount();
                    (*temp_rit).set_relativePos(orange_linecount + linecount);
                    (*temp_rit).set_handled(true);
                }
                temp_rit++;
            }
        }
        rit++;
    }

    // NOTE (kilouco): Here we'll send messages to be handled by engine, which will make text drawing.
    // This also gets info if messages should be rendered from up to down or vice-versa
    it = messages.begin();
    linecount;
    while(it != messages.end()) {
        const Position& txtpos = (*it).getPosition();
        x = ((txtpos.x - pos.x + m_vpw/2 - 2) * (m_scale * 32) + (m_scale * 16)) + walkoffx;
        y = ((txtpos.y - pos.y + m_vph/2 - 2) * (m_scale * 32)) + walkoffy;
        int screenyPos = (y - 12 - (((*it).get_relativePos()) * 12));

        // NOTE (Kilouco): This is how it will works in case the sender is too close to y = 0.
        if (screenyPos < 0) {
            // NOTE (Kilouco): A way to get "Player says: " position.
            if (((*it).get_relativePos()) > first && (*it).shouldShowName()) {
                temp_it = it;
                first = (*it).get_relativePos();
            }

            text = (*it).getText();
            y = ((linecount + 1 - (*it).get_relativePos()) * 12);
        }

        else {
            if ((*it).get_relativePos() == (linecount + orange_linecount) && (*it).shouldShowName()) {
                if ((*it).get_range() == MSG_WHISP)
                    text = (*it).getSender() + " whispers:";
                else if ((*it).get_range() == MSG_YELL)
                    text = (*it).getSender() + " yells:";
                else if ((*it).get_range() == MSG_SAY)
                    text = (*it).getSender() + " says:";
                g_engine->drawTextGW(text.c_str() , "gamefont", x, y - (((*it).get_relativePos() + 1) * 12), m_scale, (*it).getColor());
            }
            text = (*it).getText();
            y -= (((*it).get_relativePos()) * 12);
        }

        g_engine->drawTextGW(text.c_str() , "gamefont", x, y, m_scale, (*it).getColor());
        it++;
    }

    // NOTE (Kilouco): Here we actually write the "Player says: ". This works for that "y = 0" case.
    if (first > 0) {
        if((*temp_it).shouldShowName()) {
            if ((*temp_it).get_range() == 0)
                text = (*temp_it).getSender() + " whispers:";
            else if ((*it).get_range() == 2)
                text = (*temp_it).getSender() + " yells:";
            else
                text = (*temp_it).getSender() + " says:";
            g_engine->drawTextGW(text.c_str() , "gamefont", x, 0, m_scale, (*temp_it).getColor());
        }
    }

    // NOTE (Kilouco): Here we "unhandle" handled messages to be used in the next time.
    it = messages.begin();
    while(it != messages.end()) {
        (*it).set_handled(false);
        it++;
    }
}

void MapUI::drawPrivateMessages()
{
    Map::PrivateMessageList& messages = Map::getInstance().getPrivateMessages();
    Map::PrivateMessageList::iterator it = messages.begin();
    std::string text;
    double x, y;
    float scale = GlobalVariables::getScale();

    x = ((scale * 32) * 15) / 2;
    y = (((scale * 32) * 11) / 2) - ((scale * 32) * 2.5);

    if(messages.empty())
        return;

    if((*it).canBeDeleted())
        it = messages.erase(it);

    if(messages.empty())
        return;

    if(!(*it).onScreen())
        (*it).setOnScreen(true);

    y -= ((*it).getLinecount() + 1) * 6;

    text = (*it).getSender() + ":";
    g_engine->drawTextGW(text.c_str() , "gamefont", x, y - 12, scale, (*it).getColor());

    text = (*it).getText();
    g_engine->drawTextGW(text.c_str() , "gamefont", x, y, scale, (*it).getColor());
}

int MapUI::getMinZ(Position pos) {

    // NOTE (Kilouco): It works, don't ask me how. Just believe me.
	const Tile* tile = Map::getInstance().getTile(pos.x, pos.y, pos.z);
	const Tile* tile_perspective = Map::getInstance().getTile(pos.x, pos.y, pos.z);
	int minz = 0;

	// See if there is anything right above player (perspectively).
	for (int z = pos.z-1; z>=0; z--) {
        tile = Map::getInstance().getTile(pos.x, pos.y, z);
        if (tile && tile->getThingCount()) {
            minz = z+1;

            return (m_minz = minz);
        }
	}

	// Now see if is there anything above player not perspectively but just blocking his view.
	for (int z = pos.z-1; z>=0; z--) {
		tile = Map::getInstance().getTile(pos.x-(z-pos.z), pos.y-(z-pos.z), z);
		if (tile && tile->getThingCount()) {
			minz = z+1;

			return (m_minz = minz);
		}
	}

    // Last but not least: we take a look around.
	for (int z = pos.z-1; z>=0; z--) {
	    for (int y = -1; y <= 1; y++)
            for (int x = -1; x <= 1; x++) {

                if((x != 0 && y != 0) || (x == 0 && y == 0))
                    continue;

                tile = Map::getInstance().getTile(pos.x + x, pos.y + y, pos.z);
                tile_perspective = Map::getInstance().getTile(pos.x + x, pos.y + y, z);

                if (tile && tile_perspective)
                    if (tile->canSeeThrough() && (tile_perspective->getGround() || tile_perspective->getThingCount())) {
                        minz = z+1;
                        return (m_minz = minz);
                    }
	        }
	}
	return (m_minz = 0);
}

void MapUI::drawTileGhosts(int x, int y, int z, int screenx, int screeny, float scale, uint32_t tile_height)
{
    float scaledSize = std::floor(32*m_scale);
    // go through all tiles around it and see if any of the creatures is moving in direction of this tile
    for (int i = -1; i <= 1; i++)
        for (int j = -1; j <= 1; j++) {
            if (!(i || j)) continue;
            int tile_x = x+i;
            int tile_y = y+j;
            const Tile* tile = Map::getInstance().getTile(tile_x, tile_y, z);
            if (!tile) continue;
            int32_t thingsCount = tile->getThingCount() - 1;
            for (int k = 1; k < thingsCount+1; k++) {
                const Creature* c = tile->getThingByStackPos(k)->getCreature();

                int screenx2=screenx, screeny2=screeny;

                if (c && (c->getWalkState() != 1. || c->isPreWalking())) {
                    if (c->getLookDir() == DIRECTION_SOUTH && !c->isPreWalking()) continue;
                    if (c->getLookDir() == DIRECTION_NORTH && c->isPreWalking()) continue;
                    if (c->getLookDir() == DIRECTION_EAST && !c->isPreWalking()) continue;
                    if (c->getLookDir() == DIRECTION_WEST && c->isPreWalking()) continue;
                    switch (c->getLookDir()) {

                        case DIRECTION_NORTH:
                        case DIRECTION_SOUTH:
                            if (j != -1 || i != 0) continue;
                            tile_y -= 1;
                            screeny2 -= int(scaledSize);
                            break;
                        case DIRECTION_WEST:
                        case DIRECTION_EAST:
                            if (i != -1 || j != 0) continue;
                            tile_x -= 1;
                            screenx2 -= int(scaledSize);
                            break;


                        default:
                            continue;
                    }
                    c->Blit(screenx2 - (int)(tile_height*8.*m_scale),
                            screeny2 - (int)(tile_height*8.*m_scale),
                            scale, tile_x, tile_y);
                }
            }
        }

}

void MapUI::drawTileEffects(Tile* tile, int screenx, int screeny, float scale, uint32_t tile_height)
{
	Tile::EffectList& effects = const_cast<Tile*>(tile)->getEffects();
	Tile::EffectList::iterator it = effects.begin();
	while(it != effects.end()){
		if((*it)->canBeDeleted()){
			delete *it;
			effects.erase(it++);
		}
		else{
			(*it)->Blit(screenx - (int)(tile_height*8.*m_scale),
					screeny - (int)(tile_height*8.*m_scale),
					scale, 0, 0);
			++it;
		}
	}
}


void MapUI::useItem(int x, int y, const Thing* &thing, uint32_t &retx, uint32_t &rety, uint32_t &retz, int &stackpos, bool &extended)
{
	Tile* tile = translateClickToTile(x,y,retx,rety,retz);

    useItem(tile,thing,stackpos,extended);
}

void MapUI::useItem(Tile* tile, const Thing* &thing, int &stackpos, bool &extended)
{
    if (!tile) {
        thing = NULL;
        stackpos = -1;
        extended = false;
        return;
    }
	// get stackpos of thing that we clicked on
	stackpos = tile->getUseStackpos();


	// get thing that we clicked on
	thing = tile->getThingByStackPos(stackpos);

	// is this an extended use
	if (thing->getItem()->isExtendedUseable())
        extended = true;
    else
        extended = false;
}

void MapUI::useItemExtended(int x, int y, const Thing* &thing, uint32_t &retx, uint32_t &rety, uint32_t &retz, int &stackpos)
{
	Tile* tile = translateClickToTile(x,y,retx,rety,retz);

    if (!tile) {
        thing = NULL;
        stackpos = -1;
        return;
    }
	// get stackpos of thing that we clicked on
	stackpos = tile->getExtendedUseStackpos();


	// get thing that we clicked on
	thing = tile->getThingByStackPos(stackpos);
}


void MapUI::attackCreature(int x, int y, const Creature* &creature)
{
	// get the creature we're attacking from the tile we're clicking on

	if(Tile* t=translateClickToTile(x,y)){
        creature = t->getTopCreature();
	} else {
	    creature = NULL;
	}
}


void MapUI::lookAtItem(int x, int y, const Thing* &thing, uint32_t &retx, uint32_t &rety, uint32_t &retz, int &stackpos)
{
    Position p;
	Tile* tile = translateClickToTile(x,y,retx,rety,retz);

	lookAtItem(tile, thing, stackpos);
}
void MapUI::lookAtItem(Tile* tile, const Thing* &thing, int &stackpos)
{
    if(!tile){
        thing = NULL;
        stackpos = -1;
        return;
    }
	// get stackpos of thing that we clicked on
	stackpos = tile->getUseStackpos();

	// get thing that we clicked on
    thing = tile->getThingByStackPos(stackpos);

}

void MapUI::dragThing(int x, int y, const Thing*& thing, uint32_t& retx, uint32_t& rety, uint32_t& retz, int& stackpos)
{
    Position p;
	Tile* tile = translateClickToTile(x,y,retx,rety,retz);

    if(!tile){
        thing = NULL;
        stackpos = -1;
        return;
    }
	// get stackpos of thing that we clicked on
	stackpos = tile->getUseStackpos();
	// get thing that we clicked on
    thing = tile->getThingByStackPos(stackpos);
}

Tile* MapUI::translateClickToTile(int x, int y, uint32_t &retx, uint32_t &rety, uint32_t &retz)
{
    float scaledSize = std::floor(32*m_scale);
    x-=m_x; y-=m_y;
	x /= int(scaledSize); // divide by tile size
	y /= int(scaledSize); // we need the tile coordinates, not the mouse coordinates

	printf("Click on %d %d\n", x, y);
	printf("Limitx: 0 - %d\n", m_vpw);
	printf("Limity: 0 - %d\n", m_vph);

	if (x < 0 || (unsigned)x > m_vpw) {
		return NULL;
	}
	if (y < 0 || (unsigned)y > m_vph) {
		return NULL;
	}

	// get player position
	Position pos = GlobalVariables::getPlayerPosition();
	int z = pos.z;
	// get the tile we clicked on

	// NOTE (nfries88): The official client recognizes the floor above you before the floor you're on
	//	and also floors below you after.
	Tile* tile = NULL;
	int minz = 7;
	int maxz = m_minz;
	if(pos.z > 7){
		minz = pos.z + 3;
	}
	for(z = maxz; z <= minz; z++){
		tile = Map::getInstance().getTile(pos.x + x - (z-pos.z) - m_vpw/2, pos.y + y - (z-pos.z) - m_vph/2, z);
		if(tile){ // found you
			break;
		}
	}
	#ifdef DEBUG
	printf("Click on %d %d\n", x,y);
	printf("That translates into %d %d %d\n", pos.x + x - (z-pos.z) - m_vpw/2, pos.y + y - (z-pos.z) - m_vph/2, z);
	#endif

	retx = pos.x + x - (z-pos.z) - m_vpw/2;
	rety = pos.y + y - (z-pos.z) - m_vph/2;
	retz = z;

	return tile;
}

bool MapUI::handlePopup(int x, int y)
{
    float scaledSize = std::floor(32*m_scale);
    if (x > 2*scaledSize && y > 2*scaledSize && x < 2*scaledSize+15*scaledSize && y < 2*scaledSize+11*scaledSize) { // click within visible area of map?
        uint32_t retx, rety, retz;
        if(!translateClickToTile(x, y, retx, rety, retz)) return false;

        glictPos p;
        p.x = x; p.y = y;
        ((GM_Gameworld*)g_game)->performPopup(MapUI::makePopup,this,&p);
    }
	return true;
}

void MapUI::makePopup(Popup* popup, void* owner, void* arg)
{
	#define IS_LEADER(player, c, popup, s){\
		if(c->getShield() == SHIELD_NONE){\
			s.str("");\
			s << gettext("Invite to Party");\
			popup->addItem(s.str(),onInviteToParty,(void*)c->getID());\
		}\
		else if(c->getShield() == SHIELD_WHITEBLUE){\
			s.str("");\
			s << gettext("Revoke") << c->getName() << "'s " << gettext("Invitation");\
			popup->addItem(s.str(), onRevokeInvite, (void*)c->getID());\
		}\
		else if(player != c){\
			s.str("");\
			s << gettext("Pass Leadership To") << " " << c->getName();\
			popup->addItem(s.str(), onPassLeadership, (void*)c->getID());\
		}\
	}

    MapUI *m = (MapUI*)(owner);
    glictPos &pos = *((glictPos*)(arg));
    uint32_t retx, rety, retz;
	Position playerpos = GlobalVariables::getPlayerPosition();

    Tile* t = m->translateClickToTile(int(pos.x), int(pos.y), retx, rety, retz);
    if(!t) return;
    m->m_lastRightclickTilePos.x = retx;
    m->m_lastRightclickTilePos.y = rety;
    m->m_lastRightclickTilePos.z = retz;
    // TODO (ivucica#5#): if clicked on dark, right click must say "you cant see anything"
    // but first we must implement lighting at all ... :)

    std::stringstream s;

    // look at appears always
    s.str("");
    s << gettext("Look") << " (Shift)";
    popup->addItem(s.str(), MapUI::onLookAt, m);

    // use depends on kind of item
    Item* item = dynamic_cast<Item*>(t->getThingByStackPos(t->getUseStackpos()));
    s.str("");
    if (item)
    {
        if (item->isExtendedUseable())
            s << gettext("Use with...") << " (Ctrl)";
        else
            if (Objects::getInstance()->getItemType(item->getID())->container)
                s << gettext("Open") << " (Ctrl)";
            else
                s << gettext("Use") << " (Ctrl)";
        popup->addItem(s.str(), MapUI::onUse, m);
    }

    // attack depends if there is a creature and we're NOT on player pos
    if (const Creature *c = t->getTopCreature())
    {
		Creature* player = Creatures::getInstance().getPlayer();
		if(c != player) {
            m->m_popupCreatureID = c->getID();

			popup->addItem("-",NULL,NULL);
			s.str("");
			s << gettext("Attack") << " (Alt)";
			popup->addItem(s.str(),onAttack,m);

			s.str("");
			s << gettext("Follow");
			popup->addItem(s.str(),onFollow,m);
		}

        if (c->isPlayer() && (c->getCurrentPos().z == GlobalVariables::getPlayerPosition().z))
        {
            popup->addItem("-",NULL,NULL);
			if(c != player) {
				s.str("");
				s << gettext("Message to") << " " << c->getName();
				popup->addItem(s.str(),onMessageTo,m);

				s.str("");
				s << gettext("Add to VIP list");
				popup->addItem(s.str(),onAddVIP,m);

				s.str("");
				s << gettext("Ignore") << " " << c->getName();
				popup->addItem(s.str(),onUnimplemented);
			}
			else {
				s.str("");
				s << gettext("Set Outfit") << "...";
				popup->addItem(s.str(), GM_Gameworld::onSetOutfit);
			}

			switch(player->getShield())
            {
            	case SHIELD_NONE:
				{
					if(player != c && c->getShield() == SHIELD_NONE){
						s.str("");
						s << gettext("Invite to Party");
						popup->addItem(s.str(),onInviteToParty,(void*)c->getID());
					}
					else if(c->getShield() == SHIELD_WHITEYELLOW){
						s.str("");
						s << gettext("Accept") << c->getName() << "'s " << gettext("Invitation");
						popup->addItem(s.str(), onAcceptInvite, (void*)c->getID());
					}
					break;
				}
				case SHIELD_YELLOW_SHAREDEXP: case SHIELD_YELLOW_NOSHAREDEXP_BLINK: case SHIELD_YELLOW_NOSHAREDEXP:
				{
					if(player == c){
						s.str("");
						s << gettext("Leave Party");
						popup->addItem(s.str(), onLeaveParty, (void*)c->getID());

						ClientVersion_t ver = options.protocol;
						if(ver == CLIENT_VERSION_AUTO) {
							ver = ProtocolConfig::detectVersion();
						}
						if(ver >= CLIENT_VERSION_781){
							s.str("");
							s << gettext("Disable Shared Experience");
							popup->addItem(s.str(), onSharedExp, (void*)false);
						}
					}
					else IS_LEADER(player, c, popup, s)
					break;
				}
				case SHIELD_YELLOW:
				{
					if(player == c){
						s.str("");
						s << gettext("Leave Party");
						popup->addItem(s.str(), onLeaveParty, (void*)c->getID());

						ClientVersion_t ver = options.protocol;
						if(ver == CLIENT_VERSION_AUTO) {
							ver = ProtocolConfig::detectVersion();
						}
						if(ver >= CLIENT_VERSION_781){
							s.str("");
							s << gettext("Enable Shared Experience");
							popup->addItem(s.str(), onSharedExp, (void*)true);
						}
					}
					else IS_LEADER(player, c, popup, s)
					break;
				}
				default: // all the blue ones
				{
					if(player == c){
						s.str("");
						s << gettext("Leave Party");
						popup->addItem(s.str(), onLeaveParty, (void*)c->getID());
					}
					break;
				}
            }
        }

        popup->addItem("-",NULL,NULL);
        s.str("");
        s << gettext("Copy Name");
        popup->addItem(s.str(), GM_Gameworld::onCopyName, (void*)c->getID());
    }
}

void MapUI::onInviteToParty(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->m_protocol->sendInviteParty((uint32_t)VOIDP2INT(parent->data));
}
void MapUI::onRevokeInvite(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->m_protocol->sendCancelInviteParty((uint32_t)VOIDP2INT(parent->data));
}
void MapUI::onAcceptInvite(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->m_protocol->sendJoinParty((uint32_t)VOIDP2INT(parent->data));
}
void MapUI::onSharedExp(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
	gw->m_protocol->sendEnableSharedExperience((parent->data != 0), 0);
}
void MapUI::onPassLeadership(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->m_protocol->sendPassPartyLeader((uint32_t)VOIDP2INT(parent->data));
}
void MapUI::onLeaveParty(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->m_protocol->sendLeaveParty();
}

void MapUI::onLookAt(Popup::Item *parent)
{
    MapUI *m = (MapUI*)(parent->data);
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    Tile* t = Map::getInstance().getTile(m->m_lastRightclickTilePos);

    const Thing* thing;
    int stackpos;

    m->lookAtItem(t, thing, stackpos);
	if(stackpos != -1){
		gw->m_protocol->sendLookItem(t->getPos(), thing->getID(), stackpos );
	}
}

void MapUI::onUse(Popup::Item *parent)
{
    MapUI *m = (MapUI*)(parent->data);
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    Tile* t = Map::getInstance().getTile(m->m_lastRightclickTilePos);

    const Thing* thing;
    int stackpos;
    bool isextended;

    m->useItem(t, thing, stackpos, isextended);

    if(stackpos != -1){
        if(!isextended){
            gw->m_protocol->sendUseItem(t->getPos(), thing->getID(), stackpos );
        } else {
            gw->beginExtendedUse(thing,stackpos,t->getPos());
        }
    }
}

void MapUI::onAttack(Popup::Item *parent)
{
    MapUI *m = (MapUI*)(parent->data);
    GM_Gameworld *gw = (GM_Gameworld*)g_game;

    if(m->m_popupCreatureID == GlobalVariables::getAttackID()){
        gw->m_protocol->sendAttackCreature(0);
        return;
    }

    gw->m_protocol->sendAttackCreature(m->m_popupCreatureID);
    GlobalVariables::setAttackID(m->m_popupCreatureID);
}
void MapUI::onFollow(Popup::Item *parent)
{
    MapUI *m = (MapUI*)(parent->data);
    GM_Gameworld *gw = (GM_Gameworld*)g_game;

    if (GlobalVariables::getFollowID() == m->m_popupCreatureID)
    {
        gw->m_protocol->sendCancelMove();
        GlobalVariables::setFollowID(0);
    } else {
        gw->m_protocol->sendFollowCreature(m->m_popupCreatureID);
        GlobalVariables::setFollowID(m->m_popupCreatureID);
    }
}

void MapUI::onMessageTo(Popup::Item *parent)
{
    MapUI *m = (MapUI*)(parent->data);
    GM_Gameworld *gw = (GM_Gameworld*)g_game;

    gw->setActiveConsole(gw->findConsole(Creatures::getInstance().getCreature(m->m_popupCreatureID)->getName()));
}

void MapUI::onAddVIP(Popup::Item *parent)
{
    // TODO (ivucica#3#): check if cip client is making a clientside check if player is already on list
    // perhaps we should do it anyways?
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    MapUI *m = (MapUI*)(parent->data);
    Creature *c = Creatures::getInstance().getCreature(m->m_popupCreatureID);

    if (!c)
        return;
    gw->m_protocol->sendAddVip(c->getName());
}


void MapUI::onUnimplemented(Popup::Item *parent)
{
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->msgBox(gettext("This functionality is not yet finished"),"TODO");
}

std::list<Direction> MapUI::getPathTo(int scrx, int scry)
{
	std::list<Direction> path;
	uint32_t x, y, z;
	if(translateClickToTile(scrx, scry, x, y, z)) {
		path = Map::getInstance().getPathTo(x, y, z);
	}
	return path;
}
