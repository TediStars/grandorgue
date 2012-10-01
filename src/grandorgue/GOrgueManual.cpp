/*
 * GrandOrgue - free pipe organ simulator
 *
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2012 GrandOrgue contributors (see AUTHORS)
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
#include "GOrgueManual.h"

#include "GOrgueConfigReader.h"
#include "GOrgueCoupler.h"
#include "GOrgueDivisional.h"
#include "GOrgueStop.h"
#include "GrandOrgueFile.h"

GOrgueManual::GOrgueManual(GrandOrgueFile* organfile) :
	m_group(wxT("---")),
	m_midi(organfile, MIDI_RECV_MANUAL),
	m_sender(organfile, MIDI_SEND_MANUAL),
	m_organfile(organfile),
	m_KeyPressed(0),
	m_KeyState(0),
	m_manual_number(0),
	m_first_accessible_logical_key_nb(0),
	m_nb_logical_keys(0),
	m_first_accessible_key_midi_note_nb(0),
	m_nb_accessible_keys(0),
	m_UnisonOff(0),
	m_MIDIInputNumber(0),
	m_tremulant_ids(0),
	m_name(),
	m_stops(0),
	m_couplers(0),
	m_divisionals(0),
	m_displayed(false),
	m_DivisionalTemplate(organfile)
{

}

void GOrgueManual::Init(GOrgueConfigReader& cfg, wxString group, int manualNumber, unsigned first_midi, unsigned keys)
{
	m_group = group;
	m_name = wxString::Format(_("Floating %d"), manualNumber - m_organfile->GetODFManualCount() + 1);
	m_nb_logical_keys = keys;
	m_first_accessible_logical_key_nb  = 1;
	m_first_accessible_key_midi_note_nb = first_midi;
	m_nb_accessible_keys = keys;
	m_MIDIInputNumber = 0;
	m_displayed = false;
	m_manual_number = manualNumber;

	m_stops.resize(0);
	m_couplers.resize(0);
	m_tremulant_ids.resize(0);
	m_divisionals.resize(0);
	m_midi.Load(cfg, group);
	m_sender.Load(cfg, group);

	m_KeyState.resize(m_nb_logical_keys);
	std::fill(m_KeyState.begin(), m_KeyState.end(), 0);
	m_KeyPressed.resize(m_nb_accessible_keys);
	std::fill(m_KeyPressed.begin(), m_KeyPressed.end(), false);
}

void GOrgueManual::Load(GOrgueConfigReader& cfg, wxString group, int manualNumber)
{
	m_group = group;
	m_name                              = cfg.ReadString (ODFSetting, group, wxT("Name"), 32);
	m_nb_logical_keys                   = cfg.ReadInteger(ODFSetting, group, wxT("NumberOfLogicalKeys"), 1, 192);
	m_first_accessible_logical_key_nb   = cfg.ReadInteger(ODFSetting, group, wxT("FirstAccessibleKeyLogicalKeyNumber"), 1, m_nb_logical_keys);
	m_first_accessible_key_midi_note_nb = cfg.ReadInteger(ODFSetting, group, wxT("FirstAccessibleKeyMIDINoteNumber"), 0, 127);
	m_nb_accessible_keys                = cfg.ReadInteger(ODFSetting, group, wxT("NumberOfAccessibleKeys"), 0, 85);
	m_MIDIInputNumber                   = cfg.ReadInteger(ODFSetting, group, wxT("MIDIInputNumber"), 0, 200, false, 0);
	m_displayed                         = cfg.ReadBoolean(ODFSetting, group, wxT("Displayed"));
	unsigned m_nb_stops                 = cfg.ReadInteger(ODFSetting, group, wxT("NumberOfStops"), 0, 64);
	unsigned m_nb_couplers              = cfg.ReadInteger(ODFSetting, group, wxT("NumberOfCouplers"), 0, 16, false);
	unsigned m_nb_divisionals           = cfg.ReadInteger(ODFSetting, group, wxT("NumberOfDivisionals"), 0, 32, false);
	unsigned m_nb_tremulants            = cfg.ReadInteger(ODFSetting, group, wxT("NumberOfTremulants"), 0, 10, false);
	m_manual_number = manualNumber;

	m_midi.SetIndex(manualNumber);

	wxString buffer;

	m_stops.resize(0);
	for (unsigned i = 0; i < m_nb_stops; i++)
	{
		m_stops.push_back(new GOrgueStop(m_organfile, m_manual_number, GetFirstLogicalKeyMIDINoteNumber()));
		buffer.Printf(wxT("Stop%03d"), i + 1);
		buffer.Printf(wxT("Stop%03d"), cfg.ReadInteger(ODFSetting, group, buffer, 1, 448));
		m_stops[i]->Load(cfg, buffer);
	}

	m_couplers.resize(0);
	for (unsigned i = 0; i < m_nb_couplers; i++)
	{
		m_couplers.push_back(new GOrgueCoupler(m_organfile, m_manual_number));
		buffer.Printf(wxT("Coupler%03d"), i + 1);
		buffer.Printf(wxT("Coupler%03d"), cfg.ReadInteger(ODFSetting, group, buffer, 1, 64));
		m_couplers[i]->Load(cfg, buffer);
	}

	m_tremulant_ids.resize(0);
	for (unsigned i = 0; i < m_nb_tremulants; i++)
	{
		buffer.Printf(wxT("Tremulant%03d"), i + 1);
		m_tremulant_ids.push_back(cfg.ReadInteger(ODFSetting, group, buffer, 1, m_organfile->GetTremulantCount()));
	}

	m_divisionals.resize(0);
	for (unsigned i = 0; i < m_nb_divisionals; i++)
	{
		m_divisionals.push_back(new GOrgueDivisional(m_organfile, GetDivisionalTemplate()));
		buffer.Printf(wxT("Divisional%03d"), i + 1);
		buffer.Printf(wxT("Divisional%03d"), cfg.ReadInteger(ODFSetting, group, buffer, 1, 224));
		m_divisionals[i]->Load(cfg, buffer, m_manual_number, i);
	}
	m_midi.Load(cfg, group);
	m_sender.Load(cfg, group);

	m_KeyState.resize(m_nb_logical_keys);
	std::fill(m_KeyState.begin(), m_KeyState.end(), 0);
	m_KeyPressed.resize(m_nb_accessible_keys);
	std::fill(m_KeyPressed.begin(), m_KeyPressed.end(), false);
}

void GOrgueManual::LoadCombination(GOrgueConfigReader& cfg)
{
	for (unsigned i = 0; i < m_divisionals.size(); i++)
		m_divisionals[i]->LoadCombination(cfg);
}


void GOrgueManual::SetKey(unsigned note, int on, GOrgueCoupler* prev)
{
	if (note < 0 || note >= m_KeyState.size() || !on)
		return;

	m_KeyState[note] += on;

	bool unisonoff = false;
	for (unsigned i = 0; i < m_couplers.size(); i++)
	{
		if (m_couplers[i]->IsUnisonOff() && !prev && m_couplers[i]->IsEngaged())
			unisonoff = true;
		m_couplers[i]->SetKey(note, on, prev);
	}

	if (!unisonoff)
	{
		for (unsigned i = 0; i < m_stops.size(); i++)
			m_stops[i]->SetKey(note + 1, on);
	}

	if (m_first_accessible_logical_key_nb <= note + 1 && note <= m_first_accessible_logical_key_nb + m_nb_accessible_keys)
		m_organfile->ControlChanged(this);
}

#define TRIGGER_LEVEL (2<<9)

void GOrgueManual::Set(unsigned note, bool on)
{
	if (note < m_first_accessible_key_midi_note_nb || note >= m_first_accessible_key_midi_note_nb + m_KeyPressed.size())
		return;
	if (m_KeyPressed[note - m_first_accessible_key_midi_note_nb] == on)
		return;
	m_KeyPressed[note - m_first_accessible_key_midi_note_nb] = on;
	m_sender.SetKey(note, on);
	SetKey(note - m_first_accessible_key_midi_note_nb + m_first_accessible_logical_key_nb - 1, on ? TRIGGER_LEVEL : -(TRIGGER_LEVEL), NULL);
}

void GOrgueManual::SetUnisonOff(bool on)
{
	if (on)
	{
		if (m_UnisonOff++)
			return;
	}
	else
	{
		if (--m_UnisonOff)
			return;
	}
	for(unsigned i = 0; i < m_KeyPressed.size(); i++)
	{
		int note = i + m_first_accessible_logical_key_nb - 1;
		if (m_KeyPressed[i])
			for (unsigned j = 0; j < m_stops.size(); j++)
				m_stops[j]->SetKey(note + 1, on ? -TRIGGER_LEVEL : TRIGGER_LEVEL);
	}
}

GOrgueManual::~GOrgueManual(void)
{
}

GOrgueMidiReceiver& GOrgueManual::GetMidiReceiver()
{
	return m_midi;
}

GOrgueMidiSender& GOrgueManual::GetMidiSender()
{
	return m_sender;
}

const wxString& GOrgueManual::GetName()
{
	return m_name;
}

int GOrgueManual::GetMIDIInputNumber()
{
	return m_MIDIInputNumber;
}

unsigned GOrgueManual::GetLogicalKeyCount()
{
	return m_nb_logical_keys;
}

unsigned GOrgueManual::GetNumberOfAccessibleKeys()
{
	return m_nb_accessible_keys;
}

/* TODO: I suspect this could be made private or into something better... */
unsigned GOrgueManual::GetFirstAccessibleKeyMIDINoteNumber()
{
	return m_first_accessible_key_midi_note_nb;
}

int GOrgueManual::GetFirstLogicalKeyMIDINoteNumber()
{
	return m_first_accessible_key_midi_note_nb - m_first_accessible_logical_key_nb + 1;
}

unsigned GOrgueManual::GetStopCount()
{
	return m_stops.size();
}

GOrgueStop* GOrgueManual::GetStop(unsigned index)
{
	assert(index < m_stops.size());
	return m_stops[index];
}

unsigned GOrgueManual::GetCouplerCount()
{
	return m_couplers.size();
}

GOrgueCoupler* GOrgueManual::GetCoupler(unsigned index)
{
	assert(index < m_couplers.size());
	return m_couplers[index];
}

void GOrgueManual::AddCoupler(GOrgueCoupler* coupler)
{
	m_couplers.push_back(coupler);
}

unsigned GOrgueManual::GetDivisionalCount()
{
	return m_divisionals.size();
}

GOrgueDivisional* GOrgueManual::GetDivisional(unsigned index)
{
	assert(index < m_divisionals.size());
	return m_divisionals[index];
}

void GOrgueManual::AddDivisional(GOrgueDivisional* divisional)
{
	m_divisionals.push_back(divisional);
}

GOrgueCombinationDefinition& GOrgueManual::GetDivisionalTemplate()
{
	return m_DivisionalTemplate;
}

unsigned GOrgueManual::GetTremulantCount()
{
	return m_tremulant_ids.size();
}

GOrgueTremulant* GOrgueManual::GetTremulant(unsigned index)
{
	assert(index < m_tremulant_ids.size());
	return m_organfile->GetTremulant(m_tremulant_ids[index] - 1);
}

void GOrgueManual::AllNotesOff()
{
	for (unsigned j = 0; j < m_nb_accessible_keys; j++)
		Set(m_first_accessible_key_midi_note_nb + j, false);
}

bool GOrgueManual::IsKeyDown(unsigned midiNoteNumber)
{
	if (midiNoteNumber < m_first_accessible_key_midi_note_nb)
		return false;
	if (midiNoteNumber >= m_first_accessible_key_midi_note_nb + m_nb_accessible_keys)
		return false;
	if (midiNoteNumber - m_first_accessible_key_midi_note_nb + m_first_accessible_logical_key_nb - 1 >= m_KeyState.size())
		return false;
	return m_KeyState[midiNoteNumber - m_first_accessible_key_midi_note_nb + m_first_accessible_logical_key_nb - 1] > 0;
}

bool GOrgueManual::IsDisplayed()
{
	return m_displayed;
}

void GOrgueManual::Save(GOrgueConfigWriter& cfg)
{
	for (unsigned i = 0; i < m_stops.size(); i++)
		m_stops[i]->Save(cfg);

	for (unsigned i = 0; i < m_couplers.size(); i++)
		m_couplers[i]->Save(cfg);

	for (unsigned i = 0; i < m_divisionals.size(); i++)
		m_divisionals[i]->Save(cfg);

	m_midi.Save(cfg, m_group);
	m_sender.Save(cfg, m_group);
}

void GOrgueManual::Abort()
{
	AllNotesOff();

	for (unsigned i = 0; i < m_stops.size(); i++)
		m_stops[i]->Abort();

	for (unsigned i = 0; i < m_couplers.size(); i++)
		m_couplers[i]->Abort();

	for (unsigned i = 0; i < m_divisionals.size(); i++)
		m_divisionals[i]->Abort();
}

void GOrgueManual::PreparePlayback()
{
	m_KeyState.resize(m_nb_logical_keys);
	std::fill(m_KeyState.begin(), m_KeyState.end(), 0);
	m_KeyPressed.resize(m_nb_accessible_keys);
	std::fill(m_KeyPressed.begin(), m_KeyPressed.end(), false);
	m_UnisonOff = 0;

	for (unsigned i = 0; i < m_stops.size(); i++)
		m_stops[i]->PreparePlayback();

	for (unsigned i = 0; i < m_couplers.size(); i++)
		m_couplers[i]->PreparePlayback();

	for (unsigned i = 0; i < m_divisionals.size(); i++)
		m_divisionals[i]->PreparePlayback();
}

void GOrgueManual::ProcessMidi(const GOrgueMidiEvent& event)
{
	int key;

	for(unsigned i = 0; i < m_stops.size(); i++)
		m_stops[i]->ProcessMidi(event);

	for(unsigned i = 0; i < m_couplers.size(); i++)
		m_couplers[i]->ProcessMidi(event);

	for(unsigned i = 0; i < m_divisionals.size(); i++)
		m_divisionals[i]->ProcessMidi(event);

	switch(m_midi.Match(event, key))
	{
	case MIDI_MATCH_ON:
		Set(key, true);
		break;

	case MIDI_MATCH_OFF:
		Set(key, false);
		break;
		
	case MIDI_MATCH_RESET:
		AllNotesOff();
		break;
		
	default:
		break;
	}
}

void GOrgueManual::Reset()
{
	for (unsigned j = 0; j < GetCouplerCount(); j++)
                GetCoupler(j)->Reset();
	for (unsigned j = 0; j < GetDivisionalCount(); j++)
		GetDivisional(j)->Display(false);

	if (GetStopCount() == 1 && !GetStop(0)->IsDisplayed())
		return;
	
	for (unsigned j = 0; j < GetStopCount(); j++)
                GetStop(j)->Reset();
}
