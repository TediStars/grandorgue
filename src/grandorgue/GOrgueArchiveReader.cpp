/*
 * GrandOrgue - free pipe organ simulator
 *
 * Copyright 2006 Milan Digital Audio LLC
 * Copyright 2009-2015 GrandOrgue contributors (see AUTHORS)
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

#include "GOrgueArchiveReader.h"

#include "GOrgueArchiveIndex.h"
#include "GOrgueHash.h"
#include "GOrgueZipFormat.h"
#include <wx/buffer.h>
#include <wx/font.h>
#include <wx/intl.h>
#include <wx/log.h>

GOrgueArchiveReader::GOrgueArchiveReader(wxFile& file) :
	m_File(file)
{
}

GOrgueArchiveReader::~GOrgueArchiveReader()
{
}

bool GOrgueArchiveReader::Seek(size_t offset)
{
	if ((size_t)m_File.Seek(offset, wxFromStart) != offset)
	{
		wxLogError(_("Seek to offset %d in archive failed"), offset);
		return false;
	}
	return true;
}

bool GOrgueArchiveReader::Read(void* buf, size_t len)
{
	if ((size_t)m_File.Read(buf, len) != len)
	{
		wxLogError(_("Read from the archive failed (%d bytes)"), len);
		return false;
	}
	return true;
}

bool GOrgueArchiveReader::GenerateFileHash(wxString& id)
{
	unsigned length = m_File.Length();
	GOrgueHash hash;
	char buf[4096];
	if (!Seek(0))
		return false;
	for(unsigned pos = 0; pos < length; pos += sizeof(buf))
	{
		size_t l = length - pos;
		if (l > sizeof(buf))
			l = sizeof(buf);
		if (!Read(buf, l))
			return false;
		hash.Update(buf, l);
	}
	id = hash.getStringHash();
	return true;
}

size_t GOrgueArchiveReader::ExtractU64(void* ptr)
{
	uint64_t* p = (uint64_t*) ptr;
	return wxUINT64_SWAP_ON_BE( *p );
}

size_t GOrgueArchiveReader::ExtractU32(void* ptr)
{
	uint32_t* p = (uint32_t*) ptr;
	return wxUINT32_SWAP_ON_BE( *p );
}

bool GOrgueArchiveReader::ReadFileRecord(size_t central_offset, GOZipCentralHeader& central, std::vector<GOArchiveEntry>& entries)
{
	wxCharBuffer central_buf(central.name_length + central.extra_length + central.comment_length);
	if (!Seek(central_offset + sizeof(central)))
		return false;
	if (!Read(central_buf.data(), central_buf.length()))
		return false;
	size_t central_uncompressed_size = central.uncompressed_size;
	size_t central_compressed_size = central.compressed_size;
	size_t local_offset = central.offset;
	size_t central_disk_number = central.disk_number;
	{
		char * extra_ptr = central_buf.data() + central.name_length;
		size_t len = central.extra_length;
		while (len > 0)
		{
			GOZipHeaderExtraRecord extra = *(GOZipHeaderExtraRecord*)extra_ptr;
			if (len < sizeof(extra))
			{
				wxLogError(_("Incomplete central directory extra record"));
				return false;
			}
			extra.Swap();
			if (len < sizeof(extra) + extra.size)
			{
				wxLogError(_("Incomplete central directory extra record"));
				return false;
			}
			if (extra.type == 0x0001)
			{
				char* zip64_central = extra_ptr + sizeof(extra);
				size_t zip64_central_size = extra.size;
				if (central_uncompressed_size == 0xffffffff)
				{
					if (zip64_central_size < 8)
					{
						wxLogError(_("Incomplete zip64 extendend information record"));
						return false;
					}
					central_uncompressed_size = ExtractU64(zip64_central);
					zip64_central += 8;
					zip64_central_size -= 8;
				}
				if (central_compressed_size == 0xffffffff)
				{
					if (zip64_central_size < 8)
					{
						wxLogError(_("Incomplete zip64 extendend information record"));
						return false;
					}
					central_compressed_size = ExtractU64(zip64_central);
					zip64_central += 8;
					zip64_central_size -= 8;
				}
				if (local_offset == 0xffffffff)
				{
					if (zip64_central_size < 8)
					{
						wxLogError(_("Incomplete zip64 extendend information record"));
						return false;
					}
					local_offset = ExtractU64(zip64_central);
					zip64_central += 8;
					zip64_central_size -= 8;
				}
				if (central_disk_number == 0xffff)
				{
					if (zip64_central_size < 4)
					{
						wxLogError(_("Incomplete zip64 extendend information record"));
						return false;
					}
					central_disk_number = ExtractU32(zip64_central);
					zip64_central += 4;
					zip64_central_size -= 4;
				}
				if (zip64_central_size != 0)
				{
					wxLogError(_("Excess information in zip64 extendend information record"));
					return false;
				}
			} else if (extra.type == 0x0009 || extra.type == 0x000a || extra.type == 0x000c || extra.type == 0x000d || extra.type == 0x0065 || extra.type == 0x0065 ||
				   extra.type == 0x2605 || extra.type == 0x2705 || extra.type == 0x2805 || extra.type == 0x6375 || extra.type == 0x7075 || extra.type == 0xa220 ||
				   extra.type == 0x5455)
			{
			}
			else
			{
				wxLogError(_("Unknown extendend information %04x"), extra.type);
			}
			len -= sizeof(extra) + extra.size;
			extra_ptr += sizeof(extra) + extra.size;
		}
	}
	if (central_disk_number != 0)
	{
		wxLogError(_("Only non-splitted archive is supported"));
		return false;
	}
	GOZipLocalHeader local;
	if (local_offset + sizeof(local) > central_offset)
	{
		wxLogError(_("Invalid local header offset"));
		return false;
	}
	if (!Seek(local_offset))
		return false;
	if (!Read(&local, sizeof(local)))
		return false;
	local.Swap();
	if (local.signature != ZIP_LOCAL_HEADER)
	{
		wxLogError(_("Missing local header"));
		return false;
	}
	if (local_offset + local.name_length + local.extra_length + sizeof(local) > central_offset)
	{
		wxLogError(_("Incomplete local record"));
		return false;
	}
	wxCharBuffer local_buf(local.name_length + local.extra_length);
	if (!Read(local_buf.data(), local_buf.length()))
		return false;
	size_t local_uncompressed_size = local.uncompressed_size;
	size_t local_compressed_size = local.compressed_size;
	{
		char * extra_ptr = local_buf.data() + local.name_length;
		size_t len = local.extra_length;
		while (len > 0)
		{
			GOZipHeaderExtraRecord extra = *(GOZipHeaderExtraRecord*)extra_ptr;
			if (len < sizeof(extra))
			{
				wxLogError(_("Incomplete central directory extra record"));
				return false;
			}
			extra.Swap();
			if (len < sizeof(extra) + extra.size)
			{
				wxLogError(_("Incomplete central directory extra record"));
				return false;
			}
			if (extra.type == 0x0001)
			{
				char* zip64_local = extra_ptr + sizeof(extra);
				size_t zip64_local_size = extra.size;
				if (local_uncompressed_size == 0xffffffff)
				{
					if (zip64_local_size < 8)
					{
						wxLogError(_("Incomplete zip64 extendend information record"));
						return false;
					}
					local_uncompressed_size = ExtractU64(zip64_local);
					zip64_local += 8;
					zip64_local_size -= 8;
				}
				if (local_compressed_size == 0xffffffff)
				{
					if (zip64_local_size < 8)
					{
						wxLogError(_("Incomplete zip64 extendend information record"));
						return false;
					}
					local_compressed_size = ExtractU64(zip64_local);
					zip64_local += 8;
					zip64_local_size -= 8;
				}
				if (zip64_local_size != 0)
				{
					wxLogError(_("Excess information in zip64 extendend information record"));
					return false;
				}
			} else if (extra.type == 0x0009 || extra.type == 0x000a || extra.type == 0x000c || extra.type == 0x000d || extra.type == 0x0065 || extra.type == 0x0065 ||
				   extra.type == 0x2605 || extra.type == 0x2705 || extra.type == 0x2805 || extra.type == 0x6375 || extra.type == 0x7075 || extra.type == 0xa220 ||
				   extra.type == 0x5455)
			{
			}
			else
			{
				wxLogError(_("Unknown extendend information %04x"), extra.type);
			}
			len -= sizeof(extra) + extra.size;
			extra_ptr += sizeof(extra) + extra.size;
		}
	}
	if (local.version_extract != central.version_extract ||
	    local.flags != central.flags ||
	    local.compression != central.compression ||
	    local.modification_date != central.modification_date ||
	    local.modification_time != central.modification_time ||
	    local.crc != central.crc ||
	    local_uncompressed_size != central.uncompressed_size ||
	    local_compressed_size != central.compressed_size ||
	    local.name_length != central.name_length ||
	    memcmp(local_buf.data(), central_buf.data(), local.name_length))
	{
		wxLogError(_("Mismatch of local and central header"));
	}
	if (local.version_extract > 45)
	{
		wxLogError(_("ZIP uses not supported features"));
		return false;
	}
	if (local.compression != 0 || central_uncompressed_size != central_compressed_size)
	{
		wxLogError(_("Only stored compression supported"));
		return false;
	}
	if (local.flags & 0b1111011111101001)
	{
		wxLogError(_("ZIP uses not supported compression flags"));
		return false;
	}
	bool utf = local.flags & 0x0400;
	if (local_offset + central_uncompressed_size + local.name_length + local.extra_length + sizeof(local) > central_offset)
	{
		wxLogError(_("Incomplete file content"));
		return false;
	}
	static wxCSConv localConv(wxFONTENCODING_CP437);
	const wxMBConv *conv;
	if (utf)
		conv = &wxConvUTF8;
	else
		conv = &localConv;
	wxString name = wxString(local_buf.data(), *conv, local.name_length);
	if (local.name_length && name.length() == 0)
	{
		wxLogError(_("Filename contains invalid characters"));
		return false;
	}
	if (central.internal_attributes != 0)
	{
		wxLogError(_("Unknown attributes"));
	}
	if ((name.length() == 0) ||
	    (name.Find(wxT('\\')) != wxNOT_FOUND) ||
	    (name[0] == wxT('/')) ||
	    (wxIsalpha(name[0]) && name[1] == wxT(':') && name[2] == wxT('/')) ||
	    (name.Find(wxT("//")) != wxNOT_FOUND) ||
	    (name.Find(wxT("/./")) != wxNOT_FOUND) ||
	    (name.Find(wxT("/../")) != wxNOT_FOUND) ||
	    (name[0] == wxT('.') && name[1] == wxT('/')) ||
	    (name[0] == wxT('.') && name[1] == wxT('.') && name[2] == wxT('/')) ||
	    (name.length() >= 2 && name[name.length() - 1] == wxT('.') && name[name.length() - 2] == wxT('/')) ||
	    (name.length() >= 3 && name[name.length() - 1] == wxT('.') && name[name.length() - 2] == wxT('.') && name[name.length() - 3] == wxT('/')))
	{
		wxLogError(_("File name '%s' contains invalid path seperators"), name.c_str());
		return false;
	}
	if (name[name.length() - 1] == wxT('/'))
	{
		if (central_uncompressed_size != 0)
			wxLogError(_("Non-empty directory '%s'"), name.c_str());
		return true;
	}

	GOArchiveEntry e;
	e.name = name;
	e.name.Replace(wxT("/"), wxT("\\"));
	e.offset = local_offset + local.name_length + local.extra_length + sizeof(local);
	e.len = central_uncompressed_size;
	entries.push_back(e);
	return true;
}

bool GOrgueArchiveReader::ReadCentralDirectory(size_t offset, size_t entry_count, size_t length, std::vector<GOArchiveEntry>& entries)
{
	while(entry_count > 0)
	{
		GOZipCentralHeader record;
		if (length < sizeof(record))
		{
			wxLogError(_("Incomplete central directory entry"));
			return false;
		}
		if (!Seek(offset))
			return false;
		if (!Read(&record, sizeof(record)))
			return false;
		record.Swap();
		size_t len = sizeof(record);
		len += record.name_length + record.extra_length + record.comment_length;
		if (record.signature != ZIP_CENTRAL_DIRECTORY_HEADER)
		{
			wxLogError(_("Not supported entry in the central directory"));
			return false;
		}
		if (len > length)
		{
			wxLogError(_("Incomplete central directory entry"));
			return false;
		}
		if (!ReadFileRecord(offset, record, entries))
			return false;
		offset += len;
		length -= len;
		entry_count--;
	}
	if (length > 0)
	{
		wxLogError(_("Excess data in the central directory"));
		return false;
	}
	return true;
}

bool GOrgueArchiveReader::ReadEnd64Record(size_t offset, GOZipEnd64Record& record)
{
	if (12 + record.size < sizeof(record))
		return false;

	size_t size = 12 + record.size - sizeof(record);
	offset += sizeof(record);
	while (size > 0)
	{
		GOZipEnd64BlockHeader header;
		if (size < sizeof(header))
			return false;
		if (!Seek(offset))
			return false;
		if (!Read(&header, sizeof(header)))
			return false;
		header.Swap();
		if (size < sizeof(header) + header.size)
			return false;
		size -= sizeof(header) + header.size;
		offset += sizeof(header) + header.size;
	}
	return true;
}

bool GOrgueArchiveReader::ReadEndRecord(std::vector<GOArchiveEntry>& entries)
{
	GOZipEndRecord record;
	GOZipEnd64Locator locator;
	GOZipEnd64Record record64;
	bool zip64 = false;
	size_t pos = m_File.Length();
	for(unsigned i = 0; i < 0xffff; i++)
	{
		if (i + sizeof(record) > pos)
		{
			wxLogError(_("No end record found"));
			return false;
		}
		if (!Seek(pos - i - sizeof(record)))
			return false;
		if (!Read(&record, sizeof(record)))
			return false;
		record.Swap();
		if (record.signature != ZIP_END_RECORD)
			continue;
		if (record.comment_len != i)
			continue;
		pos -= i + sizeof(record);

		if (pos > sizeof(locator))
		{
			if (!Seek(pos - sizeof(locator)))
				return false;
			if (!Read(&locator, sizeof(locator)))
			    return false;
			locator.Swap();
			if (locator.signature == ZIP_END64_LOCATOR)
			{
				zip64 = true;
				if (locator.disk_count != 1 || locator.end_record_disk != 0)
				{
					wxLogError(_("Only non-splitted ZIP64 archives are supported"));
					return false;
				}
				if (locator.end_record_offset >= pos)
				{
					wxLogError(_("Invalid offset in Zip64 end locator"));
					return false;
				}
				if (!Seek(locator.end_record_offset))
					return false;
				if (!Read(&record64, sizeof(record64)))
					return false;
				record64.Swap();
				if (record64.signature != ZIP_END64_RECORD)
				{
					wxLogError(_("Zip64 end record not found"));
					return false;
				}
			}
		}
		size_t current_disk = record.current_disk;
		size_t directory_disk = record.directory_disk;
		size_t entry_count_disk = record.entry_count_disk;
		size_t entry_count = record.entry_count;
		size_t directory_size = record.directory_size;
		size_t directory_offset = record.directory_offset;
		if(zip64)
		{
			if (current_disk == 0xffff)
				current_disk = record64.current_disk;
			if (directory_disk == 0xffff)
				directory_disk = record64.directory_disk;
			if (entry_count_disk == 0xffff)
				entry_count_disk = record64.entry_count_disk;
			if (entry_count == 0xffff)
				entry_count = record64.entry_count;
			if (directory_size == 0xffffffff)
				directory_size = record64.directory_size;
			if (directory_offset == 0xffffffff)
				directory_offset = record64.directory_offset;

			if (record64.version_extract > 45)
			{
				wxLogError(_("ZIP64 uses not supported features"));
				return false;
			}
			if (record64.size + locator.end_record_offset + 12 >= pos ||
			    !ReadEnd64Record(locator.end_record_offset, record64))
			{
				wxLogError(_("Malformed zip64 end record"));
				return false;
			}
		}

		if (current_disk || directory_disk || entry_count_disk != entry_count)
		{
			wxLogError(_("Only non-splitted ZIP archives are supported"));
			return false;
		}
		return ReadCentralDirectory(directory_offset, entry_count, directory_size, entries);
	}
	wxLogError(_("No end record found"));
	return false;
}

bool GOrgueArchiveReader::ListContent(wxString& id, std::vector<GOArchiveEntry>& entries)
{
	entries.clear();
	if (!GenerateFileHash(id))
		return false;

	if (!ReadEndRecord(entries))
		return false;

	return true;
}
