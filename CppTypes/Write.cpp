#include "stdafx.h"
#include "Write.h"
#include "Utils/FileStream.h"
#include "Utils/TextReader.h"

static String lineEnds(TextReader *src) {
	String result = L"\n";
	nat pos = src->position();

	wchar prev = 0;
	while (src->more()) {
		wchar c = src->get();
		if (prev == '\r' && c == '\n') {
			result = L"\r\n";
			break;
		} else if (c == '\n') {
			result = L"\n";
			break;
		}
		prev = c;
	}

	src->seek(pos);
	return result;
}

static void generatePart(TextWriter *to, const String &indent, const String &endl, GenerateFn fn, World &world) {
	std::wostringstream tmp;
	(*fn)(tmp, world);

	String cont = tmp.str();
	bool indented = false;
	for (nat i = 0; i < cont.size(); i++) {
		if (cont[i] == '\n') {
			to->put(endl);
			indented = false;
		} else {
			if (!indented) {
				to->put(indent);
				indented = true;
			}
			to->put(cont[i]);
		}
	}
}

void generateFile(const Path &src, const Path &dest, const GenerateMap &actions, World &world) {
	TextReader *read = TextReader::create(new FileStream(src, Stream::mRead));
	TextWriter *write = TextWriter::create(new FileStream(dest, Stream::mWrite), read->format());
	String endl = lineEnds(read);

	try {
		while (read->more()) {
			String line = read->getLine();
			write->put(line);
			write->put(endl);

			nat pos = line.find(L"// ");
			if (pos != String::npos) {
				String part = line.substr(pos + 3);
				String indent = line.substr(0, pos);

				GenerateMap::const_iterator i = actions.find(part);
				if (i != actions.end()) {
					generatePart(write, indent, endl, i->second, world);
				}
			}
		}

		delete read;
		delete write;
	} catch (...) {
		delete read;
		delete write;

		// Make sure we re-run later!
		dest.deleteFile();

		throw;
	}
}
