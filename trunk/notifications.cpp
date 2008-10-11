//////////////////////////////////////////////////////////////////////
// Yet Another Tibia Client
//////////////////////////////////////////////////////////////////////
// Notifications to UI
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

#include <sstream>
#include "notifications.h"
#include "net/connection.h"
#include "gamemode.h"
extern Connection* g_connection;

std::list<std::string> g_recFiles;
std::list<std::string>::iterator g_recIt;

#include "gamecontent/container.h"
#include "gamecontent/shop.h"
#include "gamecontent/creature.h"
#include "gamecontent/globalvars.h"
#include "gamecontent/inventory.h"
#include "gamecontent/map.h"
#include "debugprint.h"
#include "util.h"

void Notifications::openCharactersList(const std::list<CharacterList_t>& list, int premDays)
{
	g_game->openCharactersList(list, premDays);
}

void Notifications::onConnectionError(int message)
{
	if(message == Connection::ERROR_CLOSED_SOCKET){
		Containers::getInstance().clear();
		Creatures::getInstance().clear();
		GlobalVariables::clear();
		Inventory::getInstance().clear();
		Map::getInstance().clear();
		g_game->onConnectionClosed();
	}
	else{
		g_game->onConnectionError(message, Connection::getErrorDesc(message));

	}
}

void Notifications::onProtocolError(bool fatal)
{
	Protocol* protocol = g_connection->getProtocol();
	std::string error = protocol->getErrorDesc();
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "\n********************************************************\n");
	if(fatal){
		DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "PROTOCOL ERROR: %s\n", error.c_str());
	}
	else{
		DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "PROTOCOL WARNING: %s\n", error.c_str());
	}
	const std::list<uint8_t>& serverCmd = protocol->getLastServerCmd();
	std::list<uint8_t>::const_iterator it;
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Last received cmd: ");
	for(it = serverCmd.begin(); it != serverCmd.end(); ++it){
		DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "%02x ", *it);
	}
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "\n");
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Total Received: %d\n", protocol->getCurrentMsgN());
	const NetworkMessage* msg = protocol->getCurrentMsg();
	if(msg){
		const unsigned char* buffer = (const unsigned char*)msg->getBuffer();
		if(buffer){
			int32_t msgRealSize = msg->getSize() - msg->getStart();
			DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "Last received msg: (%x/%x)\n", (int)((long)msg->getReadBuffer() - (long)msg->getBuffer()), msgRealSize);
			#define LINE_SIZE 16
			for(int32_t i = 0; i < msgRealSize; i += LINE_SIZE){
				int32_t pos = i;
				DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "%04x : ", i);
				for(int32_t j = 0; j < LINE_SIZE; ++j, ++pos){
					if(pos < msgRealSize){
						DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "%02x ", buffer[pos]);
					}
					else{
						DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "   ");
					}
				}
				DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "; ");
				pos = i;
				for(uint32_t j = 0; j < LINE_SIZE; ++j, ++pos){
					if(pos < msgRealSize){
						if(buffer[pos] >= 32 && buffer[pos] < 127){
							DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "%c", buffer[pos]);
						}
						else{
							DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, ".");
						}
					}
					else{
						DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, " ");
					}
				}
				DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "\n");
			}
		}
	}
	DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "********************************************************\n");
	{
        std::stringstream s;
        if (fatal)
            s << "There was a protocol error: " << error << std::endl;
        else
            s << "There was a protocol warning: " << error << std::endl;
        s << std::endl;
        s << "If you use GNU/Linux, please see your terminal for more information." << std::endl;
        s << "If you use Windows, please see stdout.txt for more information." << std::endl;
        s << std::endl;
        s << "Report this bug if you are able to reproduce it." << std::endl;
        s << "Tell us what we have to do to reproduce the bug, and provide us with" << std::endl;
        s << "last 20 lines of stdout.txt, or the output on the terminal, depending" << std::endl;
        s << "on your operating system." << std::endl;
        s << std::endl;
        NativeGUIError(s.str().c_str(), "Protocol error");
	}
}



void Notifications::openMessageWindow(WindowMessage_t type, const std::string& message)
{
	if(type == MESSAGE_MOTD){
		std::string text;
		int motdnum;
		sscanf(message.c_str(), "%d", &motdnum);

		text = message.substr(message.find('\n')+1);
		g_game->openMOTD(motdnum, text);
	}
	else{
		g_game->openMessageWindow(type, message);
	}

}


void Notifications::openWaitingList(const std::string& message, int time) {
    // TODO (ivucica#3#) ugly stub, but i'm too lazy for a proper waiting list
    std::stringstream message2;
    message2 << message << std::endl << std::endl << "Retry in " << time << "seconds";
    g_game->openMessageWindow(MESSAGE_ERROR, message2.str());

}

void Notifications::onTextMessage(MessageType_t type, const std::string& message)
{
	//DEBUGPRINT(DEBUGPRINT_LEVEL_OBLIGATORY, DEBUGPRINT_NORMAL, "%s\n", message.c_str());
	g_game->onTextMessage(type, message);
}

void Notifications::onEnterGame()
{
	g_game->onEnterGame();
}

void Notifications::onCancelWalk(Direction direction)
{
	g_game->onCancelWalk();
}

void Notifications::onCancelAttack()
{
	GlobalVariables::setAttackID(0);
}

void Notifications::onWalk()
{
	g_game->onWalk();
}

void Notifications::onCreatureSpeak(SpeakClasses_t type, int n, const std::string& name, int level, const Position& pos, const std::string& message)
{
	g_game->onCreatureSpeak(type, n, name, level, pos, message);
}

void Notifications::onCreatureSpeak(SpeakClasses_t type, int n, const std::string& name, int level, int channel, const std::string& message)
{
	g_game->onCreatureSpeak(type, n, name, level, channel, message);
}

void Notifications::onCreatureSpeak(SpeakClasses_t type, int n, const std::string& name, int level, const std::string& message)
{
	g_game->onCreatureSpeak(type, n, name, level, message);
}

void Notifications::onCreatureMove(uint32_t id)
{
	g_game->onCreatureMove(id);
}

void Notifications::onChangeStats()
{
	g_game->onChangeStats();
}

void Notifications::onChangeSkills()
{
	g_game->onChangeSkills();
}

//open/close container
void Notifications::openContainer(int cid)
{
	g_game->openContainer(cid);
}

void Notifications::closeContainer(int cid)
{
	g_game->closeContainer(cid);
}

//open/close shop
void Notifications::openShopWindow(const std::list<ShopItem>& itemlist)
{
	g_game->openShopWindow(itemlist);
}
void Notifications::closeShopWindow()
{
	g_game->closeShopWindow();
}

// when we receive player cash, we may need to update shop window
void Notifications::onUpdatePlayerCash(uint32_t newcash) {
    g_game->onUpdatePlayerCash(newcash);
}
//tutorials
void Notifications::showTutorial(uint8_t tutorialId)
{
    g_game->showTutorial(tutorialId);
}

//minimap mark
void Notifications::addMapMark(uint8_t icon, const Position& pos, const std::string& desc)
{
}

// open the outfit window with these outfits as available
void Notifications::openOutfit(const Outfit_t& current, const std::list<AvailOutfit_t>& available)
{

}
