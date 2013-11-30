/*
 * GrandOrgue - free pipe organ simulator
 *
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2013 GrandOrgue contributors (see AUTHORS)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "GOGUIHW1Background.h"
#include "GOGUIPanel.h"
#include "GOGUIDisplayMetrics.h"

GOGUIHW1Background::GOGUIHW1Background(GOGUIPanel* panel):
	GOGUIControl(panel, NULL)
{
}

void GOGUIHW1Background::Draw(wxDC* dc)
{
	m_panel->TileWood(dc, m_metrics->GetDrawstopBackgroundImageNum(), 0, 0, m_metrics->GetCenterX(), m_metrics->GetScreenHeight());
	m_panel->TileWood(dc, m_metrics->GetDrawstopBackgroundImageNum(), m_metrics->GetCenterX() + m_metrics->GetCenterWidth(), 
			  0, m_metrics->GetScreenWidth() - (m_metrics->GetCenterX() + m_metrics->GetCenterWidth()), m_metrics->GetScreenHeight());
	m_panel->TileWood(dc, m_metrics->GetConsoleBackgroundImageNum(), m_metrics->GetCenterX(), 0, m_metrics->GetCenterWidth(), m_metrics->GetScreenHeight());

	if (m_metrics->HasPairDrawstopCols())
	{
		for (unsigned i = 0; i < (m_metrics->NumberOfDrawstopColsToDisplay() >> 2); i++)
		{
			m_panel->TileWood(dc, m_metrics->GetDrawstopInsetBackgroundImageNum(), i * (2 * m_metrics->GetDrawstopWidth() + 18) + m_metrics->GetJambLeftX() - 5,
				m_metrics->GetJambLeftRightY(), 2 * m_metrics->GetDrawstopWidth() + 10, m_metrics->GetJambLeftRightHeight());
			m_panel->TileWood(dc, m_metrics->GetDrawstopInsetBackgroundImageNum(), i * (2 * m_metrics->GetDrawstopWidth() + 18) + m_metrics->GetJambRightX() - 5, 
					  m_metrics->GetJambLeftRightY(), 2 * m_metrics->GetDrawstopWidth() + 10, m_metrics->GetJambLeftRightHeight());
		}
	}

	if (m_metrics->HasTrimAboveExtraRows())
		m_panel->TileWood(dc, m_metrics->GetKeyVertBackgroundImageNum(), m_metrics->GetCenterX(), m_metrics->GetCenterY(), m_metrics->GetCenterWidth(), 8);

	if (m_metrics->GetJambTopHeight() + m_metrics->GetPistonTopHeight())
		m_panel->TileWood(dc, m_metrics->GetKeyHorizBackgroundImageNum(), m_metrics->GetCenterX(), m_metrics->GetJambTopY(), 
			 m_metrics->GetCenterWidth(), m_metrics->GetJambTopHeight() + m_metrics->GetPistonTopHeight());
	GOGUIControl::Draw(dc);
}
