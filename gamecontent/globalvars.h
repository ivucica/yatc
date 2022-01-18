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

#ifndef __GLOBALVARS_H__
#define __GLOBALVARS_H__

#include "../fassert.h"
#include "enums.h"
#include "position.h"

class GlobalVariables{
public:
	~GlobalVariables(){}

	static void clear();

	static Position& getPlayerPosition() {return m_playerPos;}

	static uint32_t getPlayerID() { return m_playerId; }
	static void setPlayerID(uint32_t v) { m_playerId = v;}

	static uint32_t getAttackID() { return m_attackId; }
	static void setAttackID(uint32_t v) { m_attackId = v;}

	static uint32_t getFollowID() { return m_followId; }
	static void setFollowID(uint32_t v) { m_followId = v;}

	static bool canReprotBugs() { return m_canReportBugs;}
	static void setCanReportBugs(bool v){ m_canReportBugs = v;}

	static uint8_t getWorldLightLevel() { return m_worldLightLevel;}
	static float getWorldLightLevelScaled() { return m_worldLightLevel / 255.f;}
	static void setWorldLightLevel(uint8_t v) { m_worldLightLevel = v; }

	static uint8_t getWorldLightColor() { return m_worldLightColor;}
	static void setWorldLightColor(uint8_t v) { m_worldLightColor = v; }

	static uint8_t getGMAction(uint32_t i){ ASSERT(i < 32); return m_GMActions[i]; }
	static void setGMAction(uint32_t i, uint8_t v){ ASSERT(i < 32); m_GMActions[i] = v; }

	static uint32_t getPlayerIcons() { return m_playerIcons;}
	static void setPlayerIcons(uint32_t v) { m_playerIcons = v;}

	static uint32_t getPlayerStat(Stat_t s);
	static void setPlayerStat(Stat_t s, uint32_t v);

	static uint32_t getPlayerSkill(SkillList_t skill, SkillAttr_t attr );
	static void setPlayerSkill(SkillList_t skill, SkillAttr_t attr, uint32_t v);

	static uint32_t getPlayerCash() { return m_cash; }
	static void setPlayerCash(uint32_t money) { m_cash = money; }

	static float getScale() { return m_scale; }
	static void setScale(float scale) { m_scale = scale; }

protected:
	GlobalVariables(){}

	static uint32_t m_playerId;
	static Position m_playerPos;
	static bool m_canReportBugs;

	static uint32_t m_attackId;
	static uint32_t m_followId;

	static uint8_t m_worldLightColor;
	static uint8_t m_worldLightLevel;

	static uint8_t m_GMActions[32];

	static uint32_t m_playerIcons;

	static uint32_t m_stats[STAT_LAST];
	static uint32_t m_skills[SKILL_LAST][SKILL_ATTR_LAST];

	static uint32_t m_cash;

	static float m_scale;
};

#endif
