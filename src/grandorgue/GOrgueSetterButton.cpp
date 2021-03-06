/*
 * GrandOrgue - free pipe organ simulator
 *
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2019 GrandOrgue contributors (see AUTHORS)
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

#include "GOrgueSetterButton.h"

#include "GOrgueSetterButtonCallback.h"
#include <wx/intl.h>

GOrgueSetterButton::GOrgueSetterButton(GrandOrgueFile* organfile, GOrgueSetterButtonCallback* setter, bool Pushbutton) :
	GOrgueButton(organfile, MIDI_RECV_SETTER, Pushbutton),
	m_setter(setter)
{
}

void GOrgueSetterButton::Push()
{
	if (m_Pushbutton)
		m_setter->SetterButtonChanged(this);
	else
		GOrgueButton::Push();
}

void GOrgueSetterButton::Set(bool on)
{
	if (IsEngaged() == on)
		return;
	m_setter->SetterButtonChanged(this);
	Display(on);
}

wxString GOrgueSetterButton::GetMidiType()
{
	return _("Button");
}
