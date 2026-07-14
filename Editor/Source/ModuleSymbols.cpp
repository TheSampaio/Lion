#include "EditorPch.h"
#include "ModuleSymbols.h"

#include <cstdint>
#include <fstream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace
{
	// A PE file addresses its own contents by RVA, which is where a section lands once the loader has
	// mapped it — not where it sits in the file. Walking the sections is what converts one to the other.
	uint32_t FileOffsetOf(uint32_t rva, const std::vector<IMAGE_SECTION_HEADER>& sections)
	{
		for (const IMAGE_SECTION_HEADER& section : sections)
		{
			const uint32_t size = std::max(section.SizeOfRawData, section.Misc.VirtualSize);

			if (rva >= section.VirtualAddress && rva < section.VirtualAddress + size)
				return section.PointerToRawData + (rva - section.VirtualAddress);
		}

		return 0;
	}

	template <typename T>
	bool ReadAt(std::fstream& file, std::streamoff offset, T& value)
	{
		file.seekg(offset, std::ios::beg);
		file.read(reinterpret_cast<char*>(&value), sizeof(T));
		return file.good();
	}

	// The CodeView record a compiler leaves behind: a tag, the identity the PDB must match, and the
	// path the debugger should look for it at. Only the path is of interest here.
	struct CodeViewHeader
	{
		uint32_t signature;   // "RSDS" for the PDB 7.0 format every current toolchain emits.
		GUID guid;
		uint32_t age;
		// A null-terminated path follows, filling the rest of the debug entry.
	};

	constexpr uint32_t kCodeViewPdb70 = 0x53445352;   // "RSDS", little-endian.
}

bool RedirectModuleSymbols(const std::string& modulePath, const std::string& symbolsFileName)
{
	std::fstream file(modulePath, std::ios::in | std::ios::out | std::ios::binary);

	if (!file.is_open())
		return false;

	IMAGE_DOS_HEADER dos{};

	if (!ReadAt(file, 0, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	uint32_t signature = 0;
	IMAGE_FILE_HEADER fileHeader{};

	if (!ReadAt(file, dos.e_lfanew, signature) || signature != IMAGE_NT_SIGNATURE)
		return false;

	if (!ReadAt(file, dos.e_lfanew + sizeof(uint32_t), fileHeader))
		return false;

	const std::streamoff optionalOffset = dos.e_lfanew + sizeof(uint32_t) + sizeof(IMAGE_FILE_HEADER);
	IMAGE_OPTIONAL_HEADER64 optional{};

	if (!ReadAt(file, optionalOffset, optional) || optional.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		return false;

	const IMAGE_DATA_DIRECTORY debug = optional.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];

	if (debug.VirtualAddress == 0 || debug.Size == 0)
		return false;

	std::vector<IMAGE_SECTION_HEADER> sections(fileHeader.NumberOfSections);
	const std::streamoff sectionsOffset = optionalOffset + fileHeader.SizeOfOptionalHeader;

	for (uint32_t i = 0; i < fileHeader.NumberOfSections; ++i)
		if (!ReadAt(file, sectionsOffset + i * sizeof(IMAGE_SECTION_HEADER), sections[i]))
			return false;

	const uint32_t directoryOffset = FileOffsetOf(debug.VirtualAddress, sections);

	if (directoryOffset == 0)
		return false;

	// A module carries several kinds of debug entry; the CodeView one is where the PDB is named.
	for (uint32_t i = 0; i < debug.Size / sizeof(IMAGE_DEBUG_DIRECTORY); ++i)
	{
		IMAGE_DEBUG_DIRECTORY entry{};

		if (!ReadAt(file, directoryOffset + i * sizeof(IMAGE_DEBUG_DIRECTORY), entry))
			return false;

		if (entry.Type != IMAGE_DEBUG_TYPE_CODEVIEW || entry.PointerToRawData == 0)
			continue;

		CodeViewHeader header{};

		if (!ReadAt(file, entry.PointerToRawData, header) || header.signature != kCodeViewPdb70)
			continue;

		// The path is patched where it sits, so it may only shrink: anything past the terminator is
		// left as it was, and a reader stops there.
		const uint32_t capacity = entry.SizeOfData - sizeof(CodeViewHeader);

		if (symbolsFileName.size() + 1 > capacity)
			return false;

		// A bare file name, so the debugger falls back to looking beside the module — which is where
		// the copy of the PDB is, and the only place it can be named without knowing the build's layout.
		file.seekp(entry.PointerToRawData + sizeof(CodeViewHeader), std::ios::beg);
		file.write(symbolsFileName.c_str(), symbolsFileName.size() + 1);

		return file.good();
	}

	return false;
}
