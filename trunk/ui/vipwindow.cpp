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


#ifdef HAVE_CONFIG_H
	#include "../config.h"
#endif
#if defined(HAVE_LIBINTL_H)
    #include <libintl.h>
#else
    #define gettext(x) (x)
#endif

#include "../gm_gameworld.h"
#include "vipwindow.h"
#include "sbvlpanel.h"
#include "../gamecontent/viplist.h"
#include "../gamecontent/creature.h"
#include "../clipboard.h"
#include "../net/protocolgame.h"

extern int g_lastmousebutton;
extern yatcClipboard g_clipboard;

winVIP_t::winVIP_t()
{
    window.SetWidth(160);
    window.SetHeight(GetDefaultHeight());
    window.SetCaption(gettext("VIP"));
    window.AddObject(&container);
    container.SetWidth(160);
    container.SetHeight(GetDefaultHeight());
    container.SetVirtualSize(160, 270);
    container.SetBGActiveness(false);

    const std::map<uint32_t, VipEntry>& entries = VipList::getInstance().getEntries();
    for (std::map<uint32_t, VipEntry>::const_iterator it = entries.begin(); it != entries.end(); it++)
    {
        addVIP(it->first);
    }
}
winVIP_t::~winVIP_t()
{
}


void winVIP_t::addVIP(uint32_t id)
{
    const std::map<uint32_t, VipEntry>& entries = VipList::getInstance().getEntries();
    std::map<uint32_t, VipEntry>::const_iterator it;

    it = entries.find(id);
    if (it == entries.end()) // if its unknown to gamecontent layer, then we cant add it either
        return;

    bool isonline = it->second.isOnline();

    glictPanel *p=m_entries[id]=new glictPanel;
    p->SetOnClick(OnListbox);
    p->SetBGActiveness(false);
    p->SetCaption(it->second.getName() + (isonline ? " *" : ""));
    p->SetCustomData(this);
    p->SetBGColor(.4,.4,.4,1.);
    if (isonline)
        p->SetCaptionColor(0,1,0);
    else {
        if(options.hideofflineVIP)
            p->SetVisible(false);
        else
            p->SetCaptionColor(1,0,0);
    }
    container.AddObject(p);
}
void winVIP_t::updateVIP(uint32_t id)
{
    const std::map<uint32_t, VipEntry>& entries = VipList::getInstance().getEntries();
    std::map<uint32_t, VipEntry>::const_iterator it;

    it = entries.find(id);
    if (it == entries.end()) // if its unknown to gamecontent layer, then we cant work with it either
        return;

    if (m_entries.find(id) == m_entries.end())
    {
        addVIP(id);
        return;
    }

    bool isonline = it->second.isOnline();

    glictPanel *p=m_entries[id];
    p->SetBGActiveness(false);
    p->SetCaption(it->second.getName());
    p->SetCustomData(this);
    p->SetBGColor(.4,.4,.4,1.);
    if (isonline)
        p->SetCaptionColor(0,1,0);
    else {
        if(options.hideofflineVIP)
            p->SetVisible(false);
        else
            p->SetCaptionColor(1,0,0);
    }
}
void winVIP_t::removeVIP(uint32_t id)
{
    glictPanel *p=m_entries[id];
    container.RemoveObject(p);
    if (m_entries.find(id) != m_entries.end())
        m_entries.erase(m_entries.find(id));

}

float winVIP_t::GetDefaultHeight()
{
    return 40.F;
}

void winVIP_t::OnClose()
{
	controller->closeVIP();
}


void winVIP_t::OnListbox(glictPos* pos, glictContainer *caller)
{
    GM_Gameworld* gw = (GM_Gameworld*)g_game;
    winVIP_t *wvip = (winVIP_t*)(caller->GetCustomData());

    std::map<uint32_t, glictPanel*>::iterator it;

    for (it = wvip->m_entries.begin(); it != wvip->m_entries.end(); it++)
    {
        it->second->SetBGActiveness(false);
    }
    ((glictPanel*)caller)->SetBGActiveness(true);

    if (g_lastmousebutton == SDL_BUTTON_RIGHT)
    {
        gw->performPopup(makeVIPPopup, (void*)wvip, (void*)caller);
    }
}

void winVIP_t::makeVIPPopup(Popup* popup, void* owner, void* arg)
{
	winVIP_t* wvip = (winVIP_t*)owner;
	glictPanel* pnl = (glictPanel*)arg;

	std::stringstream s;

	uint32_t creatureid=0;
	for(std::map<uint32_t, glictPanel*>::iterator it = wvip->m_entries.begin(); it != wvip->m_entries.end(); it++)
	{
	    if (it->second == pnl)
            creatureid = it->first;
	}
	wvip->selectedcreature=creatureid;

    s.str("");
    s << gettext("Edit ") + pnl->GetCaption();
    popup->addItem(s.str(), onUnimplemented);

    s.str("");
    s << gettext("Remove ") + pnl->GetCaption();
    popup->addItem(s.str(), onRemoveVIP, wvip);

    s.str("");
    s << gettext("Message to ") + pnl->GetCaption();
    popup->addItem(s.str(), onMessageTo, (void*)wvip->selectedcreature);

    popup->addItem("-",NULL,NULL);

    s.str("");
    s << gettext("Add new VIP");
    popup->addItem(s.str(), onUnimplemented);

    s.str("");
    s << gettext("Sort by name");
    popup->addItem(s.str(), onUnimplemented);

    s.str("");
    s << gettext("Sort by type");
    popup->addItem(s.str(), onUnimplemented);

    s.str("");
    s << gettext("Sort by status");
    popup->addItem(s.str(), onUnimplemented);

    s.str("");
    if(!options.hideofflineVIP)
        s << gettext("Hide offline VIPs");
    else
        s << gettext("Show offline VIPs");
    popup->addItem(s.str(), onHideOfflineVIPs, wvip);

    popup->addItem("-",NULL,NULL);

    s.str("");
    s << gettext("Copy name");
    popup->addItem(s.str(), onCopyName, (void*)wvip->selectedcreature);

}

void winVIP_t::onUnimplemented(Popup::Item *parent)
{
	GM_Gameworld* gw = (GM_Gameworld*)g_game;

    gw->msgBox(gettext("This functionality is not yet finished"), "TODO");
}

void winVIP_t::onRemoveVIP(Popup::Item *parent)
{
	GM_Gameworld* gw = (GM_Gameworld*)g_game;

	winVIP_t* wvip = (winVIP_t*)parent->data;
	printf("Removing %d\n", wvip->selectedcreature),
    wvip->removeVIP(wvip->selectedcreature);
    VipList::getInstance().removeEntry(wvip->selectedcreature);
    gw->m_protocol->sendRemVip(wvip->selectedcreature);
}

void winVIP_t::onHideOfflineVIPs(Popup::Item *parent)
{
    winVIP_t* wvip = (winVIP_t*)parent->data;
    std::map<uint32_t, glictPanel*>::iterator eit = wvip->m_entries.begin();

    options.hideofflineVIP = !options.hideofflineVIP;
    if(options.hideofflineVIP){
        for(; eit != wvip->m_entries.end(); eit++){
            if(!(VipList::getInstance().entryIsOnline(eit->first)))
                (eit->second)->SetVisible(false);
        }
    }
    else {
        for(; eit != wvip->m_entries.end(); eit++){
            if(!(VipList::getInstance().entryIsOnline(eit->first)))
                (eit->second)->SetVisible(true);
        }
    }
}

void winVIP_t::onMessageTo(Popup::Item *parent)
{
	VipEntry vip = VipList::getInstance().getEntry((uint32_t)VOIDP2INT(parent->data));
    GM_Gameworld *gw = (GM_Gameworld*)g_game;
    gw->setActiveConsole(gw->findConsole(vip.getName()));
}

void winVIP_t::onCopyName(Popup::Item *parent)
{
    VipEntry vip = VipList::getInstance().getEntry((uint32_t)VOIDP2INT(parent->data));
    g_clipboard.setText(vip.getName());
}
