
// Note: No matter what other statements in this file may say,
// this file is released under the same license as the latest
// version of GLICT (currently, GNU Lesser General Public License v2)

// It is not (currently) distributed as part of GLICT because UI
// element it produces is YATC-specific.


#include "stackpanel.h"
#include "util.h"
#include "gm_gameworld.h"
#include <GLICT/globals.h>
#include <string.h>
#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif
yatcStackPanel::yatcStackPanel()
{
    strcpy(this->objtype, "yatcStackPanel");

    #if (GLICT_APIREV>=95)
        SetForcedHeight(0);
        defocusabilize_element = false;
    #else
        #warning yatcStackPanel needs GLICT APIREV 95+ to function properly.
    #endif
}

yatcStackPanel::~yatcStackPanel()
{
}

/**
 * @brief Overrides the default implementation for a glictList allowing dragging
 * of children along the Y axis.
 *
 * The children are expected to be special yatcStackPanelWindow objects.
 *
 * @param evt event being dispatched to the stack panel
 * @param wparam mouse position
 * @param lparam unused
 * @param returnvalue unused for handled events; passed to glictList::CastEvent
 *                    as-is
 *
 * @return whether the event was handled
 */
bool yatcStackPanel::CastEvent(glictEvents evt, void* wparam, long lparam, void* returnvalue)
{
#if (GLICT_APIREV>=95)
    if (evt == GLICT_MOUSEMOVE) {
        if (draggedchild) {
            _updateDraggedChildPos((*(glictPos*)wparam));
            return true;
        }
    } else
    if (evt == GLICT_MOUSEUP) {
        if (draggedchild) {
            _updateDraggedChildPos((*(glictPos*)wparam));
            StopDraggingChild(*(glictPos*)wparam);
            return true;
        }
    }
#endif
    return glictList::CastEvent(evt,wparam,lparam,returnvalue);
}

void yatcStackPanel::StartDraggingChild(glictContainer* draggedChild, const glictPos &relmousepos) {
#if (GLICT_APIREV>=95)
    glictList::StartDraggingChild(draggedChild, relmousepos);
#endif
}

void yatcStackPanel::StopDraggingChild(const glictPos &eventmousepos) {
#if (GLICT_APIREV>=95)
    glictList::StopDraggingChild(eventmousepos);
    RebuildList();
#endif
}

void yatcStackPanel::ForceHeight(int height)
{
    // TODO (nfries88): close windows if needed, resize bottom remaining window appropriately.
    SetHeight(GetTotalHeight());
}

void yatcStackPanel::_updateDraggedChildPos(const glictPos &eventmousepos)
{
#if (GLICT_APIREV>=95)

    int totaldcheight = draggedchild->GetHeight() +draggedchild->GetTopSize()+draggedchild->GetBottomSize();

    //int oldy = draggedchild->GetY();
    int newy = MIN(MAX(eventmousepos.y - draggedchild->_GetDragRelMouse().y, 0), totalheight-totaldcheight);

    draggedchild->Focus(NULL);

    int currentheight=0;
    bool met = false;

    for (std::list<glictContainer*>::iterator it = listlist.begin() ; it != listlist.end() ; it++) {
        if (!(*it)->GetVisible()) continue;
        if (*it == draggedchild) {
            std::list<glictContainer*>::iterator oldit = it;
            it++;
            listlist.erase(oldit);
            if (it == listlist.end()) break;
        }
        if (*it != draggedchild) {

            int totalitheight = (*it)->GetHeight() +(*it)->GetTopSize()+(*it)->GetBottomSize();

            if (currentheight + totalitheight /2 > newy && !met) {
                currentheight += totaldcheight;
                met = true;
                listlist.insert(it, draggedchild);// inserts dragged child before "it"
            }

            (*it)->SetPos(0, currentheight);
            if (forcedheight)
                (*it)->SetHeight(forcedheight);
            (*it)->SetWidth(width - GetCurrentVScrollbarWidth() - (*it)->GetLeftSize() - (*it)->GetRightSize());
            currentheight += totalitheight;
        }

    }
    if (!met)
        listlist.push_back(draggedchild);

    draggedchild->SetPos(
        draggedchild->GetX(),
        newy
    );
#endif
}

void yatcStackPanel::Paint()
{
#if (GLICT_APIREV>=95)
    glictGlobals.PaintRect(clipleft, clipright, cliptop, MIN(clipbottom, cliptop + totalheight), glictColor(0,0,0,1));
#endif
    glictList::Paint();
}

yatcStackPanelWindow::yatcStackPanelWindow()
{
	// wide enough to display 4 items per row, tall enough to display all rows or three rows, whichever is smalle
    window.SetWidth(150 + 10); // 10 == for scrollbar
    window.SetHeight(0);

	// close button on titlebar
    // support added to glict svn 76+
    // this button needs to be relocated manually
    #if (GLICT_APIREV >= 76)
    window.AddTitlebarObject(&closebtn);
	closebtn.SetSkin(&g_skin.graphicbtn[BUTTON_CLOSE_WINDOW]);
	closebtn.SetHighlightSkin(&g_skin.graphicbth[BUTTON_CLOSE_WINDOW]);
	closebtn.SetCaption("");
	closebtn.SetWidth(12);
	closebtn.SetHeight(12);
	closebtn.SetPos(150 + 10 - 12, 2);
	closebtn.SetCustomData(this);
	closebtn.SetOnClick(OnClose);

	window.AddTitlebarObject(&btnCollapse);
	btnCollapse.SetSkin(&g_skin.graphicbtn[BUTTON_COLLAPSE_WINDOW]);
	btnCollapse.SetHighlightSkin(&g_skin.graphicbth[BUTTON_COLLAPSE_WINDOW]);
	btnCollapse.SetCaption("");
	btnCollapse.SetWidth(12);
	btnCollapse.SetHeight(12);
	btnCollapse.SetPos(150 + 10 - 24, 2);
	btnCollapse.SetCustomData(this);
	btnCollapse.SetOnClick(OnCollapse);
    #else
    #warning For titlebar objects (such as close buttons) to work properly, you need GLICT APIREV 76+
    #endif
}

void yatcStackPanelWindow::Collapse()
{
    window.SetHeight(0);
	btnCollapse.SetSkin(&g_skin.graphicbtn[BUTTON_EXPAND_WINDOW]);
	btnCollapse.SetHighlightSkin(&g_skin.graphicbth[BUTTON_EXPAND_WINDOW]);
    btnCollapse.SetOnClick(OnExpand);

    glictList* parentlist = dynamic_cast<glictList*>(window.GetParent());
    if (parentlist)
        parentlist->RebuildList();

	GM_Gameworld* gameclass = dynamic_cast<GM_Gameworld*>(g_game);
	if(gameclass)
        gameclass->updateRightSide();
}

void yatcStackPanelWindow::Expand()
{
    window.SetHeight(GetDefaultHeight());
	btnCollapse.SetSkin(&g_skin.graphicbtn[BUTTON_COLLAPSE_WINDOW]);
	btnCollapse.SetHighlightSkin(&g_skin.graphicbth[BUTTON_COLLAPSE_WINDOW]);
    btnCollapse.SetOnClick(OnCollapse);

    glictList* parentlist = dynamic_cast<glictList*>(window.GetParent());
    if (parentlist)
        parentlist->RebuildList();

	GM_Gameworld* gameclass = dynamic_cast<GM_Gameworld*>(g_game);
	if(gameclass)
        gameclass->updateRightSide();
}

void yatcStackPanelWindow::Close()
{
	OnClose();

	glictList* parentlist = dynamic_cast<glictList*>(window.GetParent());
    if (parentlist)
        parentlist->RebuildList();

	GM_Gameworld* gameclass = dynamic_cast<GM_Gameworld*>(g_game);
	if(gameclass)
        gameclass->updateRightSide();
}

void yatcStackPanelWindow::OnClose(glictPos* pos, glictContainer *caller) {
	yatcStackPanelWindow* window = (yatcStackPanelWindow*)caller->GetCustomData();
	window->Close();
}

void yatcStackPanelWindow::OnCollapse(glictPos* pos, glictContainer *caller) {
	yatcStackPanelWindow* window = (yatcStackPanelWindow*)caller->GetCustomData();
	window->Collapse();
}

void yatcStackPanelWindow::OnExpand(glictPos* pos, glictContainer *caller) {
	yatcStackPanelWindow* window = (yatcStackPanelWindow*)caller->GetCustomData();
	window->Expand();
}

void yatcStackPanelWindow::OnResized(glictSize* size, glictContainer* caller)
{
	yatcStackPanelWindow* window = (yatcStackPanelWindow*)caller->GetCustomData();

	//NativeGUIError("Holy Crap", "This works");

	// in case of horizontal resize
    window->window.SetWidth(150+10);
	window->OnResize(size->h);

    glictList* parentlist = dynamic_cast<glictList*>(window->window.GetParent());
    if (parentlist)
        parentlist->RebuildList();

	GM_Gameworld* gameclass = dynamic_cast<GM_Gameworld*>(g_game);
	if(gameclass)
        gameclass->updateRightSide();
}
