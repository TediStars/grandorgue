/*
 * GrandOrgue - free pipe organ simulator based on MyOrgan
 *
 * MyOrgan 1.0.6 Codebase - Copyright 2006 Milan Digital Audio LLC
 * MyOrgan is a Trademark of Milan Digital Audio LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef GOGUIPANELWIDGET_H
#define GOGUIPANELWIDGET_H

#include <wx/wx.h>

class GOGUIPanel;

DECLARE_EVENT_TYPE(wxEVT_GOCONTROL, -1)

class GOGUIPanelWidget : public wxPanel
{
private:
	GOGUIPanel* m_panel;
	wxBitmap m_ClientBitmap;

	void CopyToScreen(wxDC* mdc, const wxRect& rect);

public:
	GOGUIPanelWidget(GOGUIPanel* panel, wxWindow* parent, wxWindowID id = wxID_ANY);
	~GOGUIPanelWidget();

	void OnUpdate();
	void OnDraw(wxDC* dc);
	void OnErase(wxEraseEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnGOControl(wxCommandEvent& event);
	void OnMouseLeftDown(wxMouseEvent& event);
	void OnMouseRightDown(wxMouseEvent& event);
	void OnMouseScroll(wxMouseEvent& event);
	void OnKeyCommand(wxKeyEvent& event);

	DECLARE_EVENT_TABLE();
};

#endif