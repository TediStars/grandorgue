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

#include "GOrgueSound.h"

#if defined(__WXMSW__)
    #include <wx/msw/regconf.h>
#endif
#include "GrandOrgue.h"
#include "GrandOrgueFrame.h"
#include "GrandOrgueFile.h"
#include "GrandOrgueID.h"
#include "GOrguePipe.h"
#include "GOrgueStop.h"
#include "GOrgueTremulant.h"
#include "OrganDocument.h"
#include "GOrgueMidi.h"

struct_WAVE WAVE = {{'R','I','F','F'}, 0, {'W','A','V','E'}, {'f','m','t',' '}, 16, 3, 2, 44100, 352800, 8, 32, {'d','a','t','a'}, 0};

#define DELETE_AND_NULL(x) do { if (x) { delete x; x = NULL; } } while (0)

//extern GrandOrgueFile* organfile;

GOrgueSound* g_sound = 0;

DEFINE_EVENT_TYPE(wxEVT_DRAWSTOP)
DEFINE_EVENT_TYPE(wxEVT_PUSHBUTTON)
DEFINE_EVENT_TYPE(wxEVT_ENCLOSURE)
DEFINE_EVENT_TYPE(wxEVT_NOTEONOFF)
DEFINE_EVENT_TYPE(wxEVT_LISTENING)
DEFINE_EVENT_TYPE(wxEVT_METERS)
DEFINE_EVENT_TYPE(wxEVT_LOADFILE)

GOrgueSound::GOrgueSound(void) :
	sw(),
	pConfig(wxConfigBase::Get()),
	format(0),
	logSoundErrors(false),
	samplers_count(0),
	polyphony(0),
	poly_soft(0),
	volume(0),
	m_audioDevices(),
	audioDevice(NULL),
	m_samples_per_buffer(0),
	m_nb_buffers(0),
	b_limit(0),
	b_stereo(0),
	b_align(0),
	b_scale(0),
	b_random(0),
	b_stoprecording(false),
	f_output(NULL),
	meter_counter(0),
	meter_poly(0),
	meter_left(0),
	meter_right(0),
	b_active(false),
	defaultAudio(wxT(""))
{

	g_sound = this;

	try
	{

		std::vector<RtAudio::Api> rtaudio_apis;
		RtAudio::getCompiledApi(rtaudio_apis);

		for (unsigned k = 0; k < rtaudio_apis.size(); k++)
		{

			audioDevice = new RtAudio(rtaudio_apis[k]);
			for (unsigned i = 0; i < audioDevice->getDeviceCount(); i++)
			{

				RtAudio::DeviceInfo info = audioDevice->getDeviceInfo(i);
				wxString dev_name = wxString::FromAscii(info.name.c_str());
				dev_name.Replace(wxT("\\"), wxT("|"));
				wxString name = wxString(GOrgueRtHelpers::GetApiName(rtaudio_apis[k])) + wxString(wxT(": ")) + dev_name;

				if (info.isDefaultOutput && defaultAudio.IsEmpty())
					defaultAudio = name;

				unsigned sample_rate_index = info.sampleRates.size();
				if (rtaudio_apis[k] != RtAudio::WINDOWS_DS)
				{
					/* TODO: can a Windows developer explain why this is necessary? */
					for (unsigned j = 0; j < sample_rate_index; j++)
						if (info.sampleRates[j] == 44100)
							sample_rate_index = j;
				}
				else
				{
					sample_rate_index = 0;
					if (info.sampleRates.size() && info.sampleRates.back() < 44100)
						sample_rate_index = info.sampleRates.size();
				}

				if (
						(info.outputChannels < 2) ||
						(!info.probed) ||
						(sample_rate_index == (int)info.sampleRates.size()) ||
						(m_audioDevices.find(name) != m_audioDevices.end())
					)
					continue;

				GO_SOUND_DEV_CONFIG cfg;
				cfg.rt_api = rtaudio_apis[k];
				cfg.rt_api_subindex = i;
				m_audioDevices[name] = cfg;

			}

			delete audioDevice;
			audioDevice = 0;

		}

		m_midi = new GOrgueMidi();

	}
	catch (RtError &e)
	{
		e.printMessage();
		CloseSound(NULL);
	}

}

GOrgueSound::~GOrgueSound(void)
{

	CloseSound(NULL);

	/* dispose of midi devices */
	DELETE_AND_NULL(m_midi);

	try
	{

		/* dispose of the audio playback device */
		DELETE_AND_NULL(audioDevice);

	}
	catch (RtError &e)
	{
		e.printMessage();
	}

}

bool GOrgueSound::OpenSound(bool wait, GrandOrgueFile* organfile)
{
	int i;

	defaultAudio = pConfig->Read(wxT("Devices/DefaultSound"), defaultAudio);
	volume = pConfig->Read(wxT("Volume"), 50);
	polyphony = pConfig->Read(wxT("PolyphonyLimit"), 2048);
	poly_soft = (polyphony * 3) / 4;
	b_stereo = pConfig->Read(wxT("StereoEnabled"), 1);
	b_limit  = pConfig->Read(wxT("ManagePolyphony"), 1);
	b_align  = pConfig->Read(wxT("AlignRelease"), 1);
	b_scale  = pConfig->Read(wxT("ScaleRelease"), 1);
	b_random = pConfig->Read(wxT("RandomizeSpeaking"), 1);

	samplers_count = 0;
	for (i = 0; i < MAX_POLYPHONY; i++)
		samplers_open[i] = samplers + i;
	for (i = 0; i < 26; i++)
		windchests[i] = 0;

	try
	{

		m_midi->Open();

		std::map<wxString, GO_SOUND_DEV_CONFIG>::iterator it;
		it = m_audioDevices.find(defaultAudio);
		if (it != m_audioDevices.end())
		{

			audioDevice = new RtAudio(it->second.rt_api);

			unsigned try_latency = pConfig->Read(wxT("Devices/Sound/") + defaultAudio, 12);
			GOrgueRtHelpers::GetBufferConfig(it->second.rt_api, try_latency, &m_nb_buffers, &m_samples_per_buffer);

			RtAudio::StreamParameters aOutputParam;
			aOutputParam.deviceId = it->second.rt_api_subindex;
			aOutputParam.nChannels = 2; //stereo

			RtAudio::StreamOptions aOptions;
			aOptions.numberOfBuffers = m_nb_buffers;

            format = RTAUDIO_FLOAT32;
			audioDevice->openStream
				(&aOutputParam
				,NULL
				,format
				,44100
				,&m_samples_per_buffer
				,&GOrgueSound::AudioCallback
				,this
				,&aOptions
				);

			m_nb_buffers = aOptions.numberOfBuffers;

			if (m_samples_per_buffer <= 1024)
			{
				audioDevice->startStream();
				pConfig->Write(wxT("Devices/Sound/ActualLatency/") + defaultAudio, GetLatency());
			}
			else
			{
				::wxSleep(1);
				if (logSoundErrors)
					::wxLogError(wxT("Cannot use buffer size above 1024 samples; unacceptable quantization would occur."));
				CloseSound(organfile);
				return false;
			}

		}

		if (!m_midi->HasActiveDevice() || it == m_audioDevices.end())
		{

			::wxSleep(1);
			if (logSoundErrors)
				::wxLogWarning
					( m_midi->HasActiveDevice()
					? wxT("No audio device is selected; neither MIDI input nor sound output will occur!")
					: wxT("No MIDI devices are selected for listening; neither MIDI input nor sound output will occur!")
					);
			CloseSound(organfile);
			return false;

		}

	}
	catch (RtError &e)
	{
		::wxSleep(1);
		if (logSoundErrors)
            e.printMessage();
		CloseSound(organfile);
		return false;
	}

	if (wait)
		::wxSleep(1);

	return true;
}

void GOrgueSound::CloseSound(GrandOrgueFile* organfile)
{
	bool was_active = b_active;

	if (f_output)
	{
		b_stoprecording = true;
		::wxMilliSleep(100);
		CloseWAV();		// this should never be necessary...
	}

	b_active = false;

	try
	{
		if (audioDevice)
		{
			audioDevice->abortStream();
			audioDevice->closeStream();
			delete audioDevice;
			audioDevice = 0;
		}
	}
	catch (RtError &e)
	{
		e.printMessage();
	}

	::wxMilliSleep(10);
	if (was_active)
		MIDIAllNotesOff(organfile);
}

bool GOrgueSound::ResetSound(GrandOrgueFile* organfile)
{
	wxBusyCursor busy;
	bool was_active = b_active;

	int temp = volume;
	CloseSound(organfile);
	if (!OpenSound(true, organfile))
		return false;
	if (!temp)  // don't let resetting sound reactivate an organ
        g_sound->volume = temp;
	b_active = was_active;
	if (organfile)
	{
        OrganDocument* doc = (OrganDocument*)(::wxGetApp().m_docManager->GetCurrentDocument());
        if (doc && doc->b_loaded)
        {
            b_active = true;
            for (int i = 0; i < organfile->GetTremulantCount(); i++)
            {
                if (organfile->GetTremulant(i)->DefaultToEngaged)
                {
                    organfile->GetTremulant(i)->Set(false);
                    organfile->GetTremulant(i)->Set(true);
                }
            }
        }
	}
	return true;
}

void GOrgueSound::CloseWAV()
{
	if (!f_output)
		return;
	WAVE.Subchunk2Size = (WAVE.ChunkSize = ftell(f_output) - 8) - 36;
    fseek(f_output, 0, SEEK_SET);
	fwrite(&WAVE, sizeof(WAVE), 1, f_output);
	fclose(f_output);
	b_stoprecording = false;
	f_output = 0;
}

void GOrgueSound::SetPolyphonyLimit(int polyphony)
{
	this->polyphony = polyphony;
}

void GOrgueSound::SetPolyphonySoftLimit(int polyphony_soft)
{
	this->poly_soft = polyphony_soft;
}

void GOrgueSound::SetVolume(int volume)
{
	this->volume = volume;
}

GO_SAMPLER* GOrgueSound::OpenNewSampler()
{

	if (samplers_count >= polyphony)
		return NULL;

	GO_SAMPLER* sampler = samplers_open[samplers_count++];
	memset(sampler, 0, sizeof(GO_SAMPLER));
	return sampler;

}

bool GOrgueSound::HasRandomPipeSpeech()
{
	return b_random;
}

bool GOrgueSound::HasReleaseAlignment()
{
	return b_align;
}

bool GOrgueSound::HasScaledReleases()
{
	return b_scale;
}

void GOrgueSound::MIDIAllNotesOff(GrandOrgueFile* organfile)
{
	int i, j, k;

	if (!organfile)
		return;

	for (i = organfile->GetFirstManualIndex(); i <= organfile->GetManualAndPedalCount(); i++)
	{
		organfile->GetManual(i)->AllNotesOff();
		for (j = 0; j < organfile->GetManual(i)->GetStopCount(); j++)
		{
			for (k = 0; k < organfile->GetManual(i)->GetStop(j)->NumberOfLogicalPipes; k++)
				organfile->GetManual(i)->GetStop(j)->GetPipe(k)->FastAbort();
			if (organfile->GetManual(i)->GetStop(j)->m_Auto)
				organfile->GetManual(i)->GetStop(j)->Set(false);
		}
	}
}

bool GOrgueSound::IsStereo()
{
	return b_stereo;
}

int GOrgueSound::GetVolume()
{
	return volume;
}

bool GOrgueSound::IsRecording()
{
	return f_output && !b_stoprecording;
}

/* FIXME: This code is not thread-safe and is likely to cause future problems */
void GOrgueSound::StartRecording()
{
	if (f_output)
        return;
	b_stoprecording = false;
	wxFileDialog dlg(::wxGetApp().frame, _("Save as"), wxConfig::Get()->Read(wxT("wavPath"), ::wxGetApp().m_path), wxEmptyString, _("WAV files (*.wav)|*.wav"), wxSAVE | wxOVERWRITE_PROMPT);
	if (dlg.ShowModal() == wxID_OK)
	{
        wxConfig::Get()->Write(wxT("wavPath"), dlg.GetDirectory());
        wxString filepath = dlg.GetPath();
        if (filepath.Find(wxT(".wav")) == wxNOT_FOUND) {
            filepath.append(wxT(".wav"));
        }
        if ((f_output = fopen(filepath.mb_str(), "wb")))
            fwrite(&WAVE, sizeof(WAVE), 1, f_output);
        else
            ::wxLogError(wxT("Unable to open file for writing"));

	}
}

void GOrgueSound::StopRecording()
{

	if (!f_output)
		return;

	b_stoprecording = true;

}

bool GOrgueSound::IsActive()
{
	return b_active;
}

void GOrgueSound::ActivatePlayback()
{
	/* FIXME: we should probably check that the driver is actually open */
	b_active = true;
}

void GOrgueSound::SetLogSoundErrorMessages(bool settingsDialogVisible)
{
	logSoundErrors = settingsDialogVisible;
}

std::map<wxString, GOrgueSound::GO_SOUND_DEV_CONFIG>& GOrgueSound::GetAudioDevices()
{
	return m_audioDevices;
}

const int GOrgueSound::GetLatency()
{

	if (!audioDevice)
		return -1;

	int actual_latency = audioDevice->getStreamLatency();

	/* getStreamLatency returns zero if not supported by the API, in which
	 * case we will make a best guess.
	 */
	if (actual_latency == 0)
		actual_latency = m_samples_per_buffer * m_nb_buffers;

	return (actual_latency * 1000) / 44100;

}

const wxString GOrgueSound::GetDefaultAudioDevice()
{
	return defaultAudio;
}

const RtAudioFormat GOrgueSound::GetAudioFormat()
{
	return format;
}

GOrgueMidi& GOrgueSound::GetMidi()
{
	return *m_midi;
}
