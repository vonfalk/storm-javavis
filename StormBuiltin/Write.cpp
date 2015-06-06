#include "stdafx.h"
#include "Write.h"
#include "Utils/FileStream.h"
#include "Utils/TextReader.h"
#include "StreamBuffer.h"

using namespace util;

struct FileFmt {
	textfile::Format fmt;
	String newline;
};

String indentation(const String &line) {
	for (nat i = 0; i < line.size(); i++) {
		switch (line[i]) {
		case ' ':
		case '\t':
			break;
		default:
			return line.substr(0, i);
		}
	}
	return line;
}

void addLines(vector<String> &to, const String &from, const String &prepend) {
	vector<String> l = from.split(L"\n");
	for (nat i = 0; i < l.size(); i++)
		to.push_back(prepend + l[i]);
}

FileFmt fileFmt(const Path &file) {
	FileFmt res = { textfile::utf8, L"\n" };
	TextReader *r = TextReader::create(new FileStream(file, Stream::mRead));
	res.fmt = r->format();
	String &result = res.newline;

	wchar prev = 0;
	while (r->more()) {
		wchar c = r->get();
		if (prev == '\r' && c == '\n') {
			result = L"\r\n";
			break;
		} else if (c == '\n') {
			result = L"\n";
			break;
		}
		prev = c;
	}

	delete r;

	return res;
}

vector<String> readFile(const Path &file, const FileData &data) {
	vector<String> lines;

	TextReader *r = TextReader::create(new FileStream(file, Stream::mRead));
	bool keep = true;

	while (r->more()) {
		String line = r->getLine();
		if (line.find(L"// BEGIN LIST") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, data.functionList, indentation(line));
		} else if (line.find(L"// END LIST") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN INCLUDES") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, data.headerList, indentation(line));
		} else if (line.find(L"// END INCLUDES") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN TYPES") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, data.typeList, indentation(line));
		} else if (line.find(L"// END TYPES") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN STATIC") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, data.typeFunctions, indentation(line));
		} else if (line.find(L"// END STATIC") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN VARS") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, data.variableList, indentation(line));
		} else if (line.find(L"// END VARS") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (line.find(L"// BEGIN THREADS") != String::npos) {
			keep = false;
			lines.push_back(line);
			addLines(lines, data.threadList, indentation(line));
		} else if (line.find(L"// END THREADS") != String::npos) {
			keep = true;
			lines.push_back(line);
		} else if (keep) {
			lines.push_back(line);
		}
	}

	textfile::Format fmt = r->format();
	delete r;

	return lines;
}

void writeFile(const Path &to, const vector<String> &lines, FileFmt format) {
	Stream *s = new StreamBuffer(new FileStream(to, Stream::mWrite));
	TextWriter *w = TextWriter::create(s, format.fmt);

	for (nat i = 0; i < lines.size(); i++) {
		if (i > 0)
			w->put(format.newline);
		w->put(lines[i]);
	}

	delete w;
}

void update(const Path &inFile, const Path &outFile, const Path &asmFile, const FileData &data) {
	FileFmt fmt = fileFmt(inFile);

	{
		vector<String> lines = readFile(inFile, data);
		writeFile(outFile, lines, fmt);
	}

	{
		vector<String> lines;
		addLines(lines, data.vtableCode, L"");
		writeFile(asmFile, lines, fmt);
	}

}
