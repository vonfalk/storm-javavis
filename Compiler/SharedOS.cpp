#include "stdafx.h"
#include "SharedOS.h"
#include "Core/Io/Stream.h"

namespace storm {

#ifdef WINDOWS

	LoadedLib invalidLib = NULL;

	LoadedLib loadLibrary(const wchar *path) {
		return LoadLibrary(path);
	}

	void unloadLibrary(LoadedLib lib) {
		FreeLibrary(lib);
	}

	const void *findLibraryFn(LoadedLib lib, const char *name) {
		return GetProcAddress(lib, name);
	}

	// Fill a structure with data from a stream.
	template <class T>
	static bool fill(IStream *src, T &out) {
		GcPreArray<byte, sizeof(T)> data;
		Buffer r = src->read(emptyBuffer(data));
		if (r.filled() != sizeof(T))
			return false;
		memcpy(&out, r.dataPtr(), sizeof(T));
		return true;
	}

	// Read data to an array.
	template <class T>
	T *read(IStream *src, Nat count) {
		Nat size = count*sizeof(T);
		Buffer r = src->read(buffer(src->engine(), size));
		if (r.filled() != size)
			return null;
		return (T *)r.dataPtr();
	}

	// Find the file offset of a specific section.
	bool seekSection(RIStream *stream, size_t address, IMAGE_SECTION_HEADER *sections, Nat numSections) {
		Nat use = numSections;
		for (Nat i = 0; i < numSections; i++) {
			if (sections[i].VirtualAddress > address)
				break;
			use = i;
		}

		if (use >= numSections)
			return false;

		address -= sections[use].VirtualAddress;
		address += sections[use].PointerToRawData;
		stream->seek(address);

		return true;
	}

	static bool hasExport(RIStream *src, const char *findName) {
		IMAGE_DOS_HEADER dosHeader;
		if (!fill(src, dosHeader))
			return false;

		if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE || dosHeader.e_lfanew == 0)
			return false;

		src->seek(dosHeader.e_lfanew);
		IMAGE_NT_HEADERS ntHeader;
		if (!fill(src, ntHeader))
			return false;

		if (ntHeader.Signature != IMAGE_NT_SIGNATURE || ntHeader.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
			return false;

		// Find and load the section table.
		Nat sectionCount = ntHeader.FileHeader.NumberOfSections;
		if (sectionCount > 1000)
			return false;
		IMAGE_SECTION_HEADER *sections = read<IMAGE_SECTION_HEADER>(src, sectionCount);
		if (!sections)
			return false;

		IMAGE_DATA_DIRECTORY &exports = ntHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		if (exports.VirtualAddress == 0)
			return false;

		if (!seekSection(src, exports.VirtualAddress, sections, sectionCount))
			return false;
		IMAGE_EXPORT_DIRECTORY exportDir;
		if (!fill(src, exportDir))
			return false;

		// Search for names... TODO: Check the pointer size on 64-bit systems. Could be either 32-bit or 64-bit!
		Nat nameCount = exportDir.NumberOfNames;
		if (!seekSection(src, exportDir.AddressOfNames, sections, sectionCount))
			return false;
		DWORD *names = read<DWORD>(src, nameCount);
		if (!names)
			return false;

		// Iterate through the names and see what we find!
		size_t findSize = strlen(findName);
		for (Nat i = 0; i < nameCount; i++) {
			if (!seekSection(src, names[i], sections, sectionCount))
				return false;
			char *data = read<char>(src, findSize + 1);
			if (!data)
				return false;

			if (strncmp(findName, data, findSize + 1) == 0)
				return true;
		}

		return true;
	}

	bool hasExport(Url *file, const char *name) {
		RIStream *src = file->read()->randomAccess();
		bool r = hasExport(src, name);
		src->close();
		return r;
	}

#else
#error "Implement 'loadLibrary', 'unloadLibrary' and 'findLibraryFn'!"
#endif

}
