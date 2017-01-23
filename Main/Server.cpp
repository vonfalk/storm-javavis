#include "stdafx.h"
#include "Server.h"

using namespace storm;

int runServer(Engine &e) {
	static const byte utf8Data2[] = {
		0x61, // a
		0xC3, 0x96, // 0xD6
		0xE0, 0xB4, 0xB6, // 0x0D36
		0xE3, 0x81, 0x82, // 0x3042
		0xE7, 0xA7, 0x81, // 0x79C1
		0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
		0x65, // e

		// The expression (1 "string" (2 3)) encoded.
		0x00, // start of command
		0x01, // cons
		0x02, 0x01, 0x00, 0x00, 0x00, // 1
		0x01, // cons
		0x03, 0x06, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, // string "string"
		0x01, // cons
		0x01, // cons
		0x02, 0x02, 0x00, 0x00, 0x00, // 2
		0x01, // cons
		0x02, 0x03, 0x00, 0x00, 0x00, // 3
		0x00, // nil
		0x00, // nil
	};

	PLN(L"Welcome to the language server!");

	for (nat i = 0; i < ARRAY_COUNT(utf8Data2); i++) {
		Sleep(200);
		wcout << (char)utf8Data2[i] << std::flush;
	}
	return 0;
}
