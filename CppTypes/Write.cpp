#include "stdafx.h"
#include "Write.h"
#include "Exception.h"
#include "World.h"
#include "Utils/FileStream.h"
#include "Utils/MemoryStream.h"
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
				// Do not indent preprocessor directives!
				if (cont[i] != '#')
					to->put(indent);
				indented = true;
			}
			to->put(cont[i]);
		}
	}
}

static String findIndentation(const String &line) {
	for (nat i = 0; i < line.size(); i++) {
		switch (line[i]) {
		case ' ':
		case '\t':
			break;
		default:
			return line.substr(0, i);
		}
	}
	return L"";
}

void generateFile(const Path &src, const Path &dest, const GenerateMap &actions, World &world) {
	{
		Path folder = dest.parent();
		if (!folder.exists())
			folder.createDir();
	}

	FileStream *rStream = new FileStream(src, Stream::mRead);
	if (!rStream->valid()) {
		delete rStream;
		throw Error(L"Failed to open the template file: " + toS(src), SrcPos());
	}
	TextReader *read = TextReader::create(rStream);
	TextWriter *write = TextWriter::create(new FileStream(dest, Stream::mWrite), true, read->format());
	String endl = lineEnds(read);

	try {
		while (read->more()) {
			String line = read->getLine();
			write->put(line);
			write->put(endl);

			nat pos = line.find(L"// ");
			if (pos != String::npos) {
				String part = line.substr(pos + 3);
				String indent = findIndentation(line);

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

template <class T>
const String &get(const NameMap<T> &src, nat id) {
	return src[id]->doc;
}

template <class T>
const String &get(const vector<T> &src, nat id) {
	return src[id].doc;
}

template <class T>
vector<nat> writeObjects(Stream &to, const T &src) {
	vector<nat> result;
	textfile::Utf8Writer w(&to, false, false);

	for (nat i = 0; i < src.size(); i++) {
		result.push_back(nat(to.pos()));
		w.put(get(src, i));
	}

	// Extra entry so that we know where the last string ends.
	result.push_back(nat(to.pos()));

	return result;
}

void generateDoc(const Path &out, World &world) {
	{
		Path folder = out.parent();
		if (!folder.exists())
			folder.createDir();
	}

	MemoryStream data;
	vector<vector<nat>> index;

	index.push_back(writeObjects(data, world.types));
	index.push_back(writeObjects(data, world.functions));
	index.push_back(writeObjects(data, world.templates));
	index.push_back(writeObjects(data, world.threads));

	FileStream file(out, Stream::mWrite);
	if (!file.valid())
		throw Error(L"Could not write to: " + toS(out), SrcPos());

	// Write a table containing the offsets to the sub-tables.
	nat indexCount = 0;
	for (nat i = 0; i < index.size(); i++) {
		file.write(sizeof(nat), &indexCount);

		indexCount += index[i].size();
	}
	// Total number of entries ends the initial table.
	file.write(sizeof(nat), &indexCount);

	// Offset all tables to accommodate for the sub-tables and write out all tables.
	indexCount += index.size()+1;
	for (nat i = 0; i < index.size(); i++) {
		for (nat j = 0; j < index[i].size(); j++)
			index[i][j] += indexCount*sizeof(nat);

		file.write(sizeof(nat) * index[i].size(), &index[i][0]);
	}

	file.write(&data);
}
