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

#include "GOSoundReverbEngine.h"

#include "GOSoundReverbPartition.h"
#include <algorithm>

GOSoundReverbEngine::GOSoundReverbEngine(unsigned samples_per_buffer) :
	m_Partitions()
{
	if (samples_per_buffer < 128)
	{
		m_Partitions.push_back(new GOSoundReverbPartition(64, 3, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(128, 6, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(512, 6, 64));
		m_Partitions.push_back(new GOSoundReverbPartition(2048, 6, 64 + 256));
		m_Partitions.push_back(new GOSoundReverbPartition(8192, 121, 64 + 256 + 1024));
	}
	else if (samples_per_buffer < 256)
	{
		m_Partitions.push_back(new GOSoundReverbPartition(128, 7, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(512, 6, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(2048, 6, 256));
		m_Partitions.push_back(new GOSoundReverbPartition(8192, 121, 256 + 1024));
	}
	else if (samples_per_buffer < 512)
	{
		m_Partitions.push_back(new GOSoundReverbPartition(256, 3, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(512, 6, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(2048, 6, 256));
		m_Partitions.push_back(new GOSoundReverbPartition(8192, 121, 256 + 1024));
	}
	else if (samples_per_buffer < 1024)
	{
		m_Partitions.push_back(new GOSoundReverbPartition(512, 7, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(2048, 6, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(8192, 121, 1024));
	}
	else
	{
		m_Partitions.push_back(new GOSoundReverbPartition(1024, 3, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(2048, 6, 0));
		m_Partitions.push_back(new GOSoundReverbPartition(8192, 121, 1024));
	}
}

GOSoundReverbEngine::~GOSoundReverbEngine()
{
}

void GOSoundReverbEngine::Reset()
{
	for(unsigned i = 0; i < m_Partitions.size(); i++)
		m_Partitions[i]->Reset();
}

void GOSoundReverbEngine::Process(float* output_buf, const float* input_buf, unsigned len)
{
	std::fill(output_buf, output_buf + len, 0);
	for(unsigned i = 0; i < m_Partitions.size(); i++)
		m_Partitions[i]->Process(output_buf, input_buf, len);
}

void GOSoundReverbEngine::AddIR(const float* data, unsigned pos, unsigned len)
{
	unsigned offset = 0;
	for(unsigned i = 0; i < m_Partitions.size(); i++)
	{
		m_Partitions[i]->AddIR(data, pos, len, offset);
		offset += m_Partitions[i]->GetLength();
	}
}

