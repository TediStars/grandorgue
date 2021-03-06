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

#ifndef MIDIEVENTSENDDIALOG_H_
#define MIDIEVENTSENDDIALOG_H_

#include "GOrgueChoice.h"
#include "GOrgueMidiSender.h"
#include <wx/panel.h>

class GOrgueSettings;
class wxButton;
class wxChoice;
class wxSpinCtrl;
class wxStaticText;
class MIDIEventRecvDialog;

class MIDIEventSendDialog : public wxPanel
{
private:
	GOrgueSettings& m_Settings;
	GOrgueMidiSender* m_original;
	MIDIEventRecvDialog* m_recv;
	GOrgueMidiSenderData m_midi;
	GOrgueChoice<midi_send_message_type>* m_eventtype;
	wxChoice *m_eventno, *m_channel, *m_device;
	wxStaticText* m_KeyLabel;
	wxSpinCtrl *m_key;
	wxStaticText* m_LowValueLabel;
	wxSpinCtrl *m_LowValue;
	wxStaticText* m_HighValueLabel;
	wxSpinCtrl *m_HighValue;
	wxStaticText* m_StartLabel;
	wxSpinCtrl *m_StartValue;
	wxStaticText* m_LengthLabel;
	wxSpinCtrl *m_LengthValue;
	wxButton* m_new, *m_delete, *m_copy;
	int m_current;

	void StoreEvent();
	void LoadEvent();
	MIDI_SEND_EVENT CopyEvent();

	void OnNewClick(wxCommandEvent& event);
	void OnDeleteClick(wxCommandEvent& event);
	void OnEventChange(wxCommandEvent& event);
	void OnTypeChange(wxCommandEvent& event);
	void OnCopyClick(wxCommandEvent& event);

protected:
	enum {
		ID_EVENT_NO = 200,
		ID_EVENT_NEW,
		ID_EVENT_DELETE,
		ID_DEVICE,
		ID_EVENT,
		ID_CHANNEL,
		ID_KEY,
		ID_LOW_VALUE,
		ID_HIGH_VALUE,
		ID_START,
		ID_LENGTH,
		ID_COPY
	};

public:
	MIDIEventSendDialog (wxWindow* parent, GOrgueMidiSender* event, MIDIEventRecvDialog* recv, GOrgueSettings& settings);
	~MIDIEventSendDialog();

	void DoApply();

	DECLARE_EVENT_TABLE()
};

#endif
