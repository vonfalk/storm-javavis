#include "stdafx.h"
#include "Main.h"
#include "Engine.h"
#include "Core/Io/StdStream.h"
#include "Core/Io/FileStream.h"

namespace storm {

	void runLangServer(Engine &e) {
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

		OStream *to = proc::out(e);
		// OStream *to = new (e) OFileStream(new (e) Str(L"C:\\Users\\Filip\\Test.txt"));

		to->write(buffer(e, (byte *)"Welcome to the language server!\n", 32));

		// PLN(L"Welcome to the language server!");

		// for (nat i = 0; i < 20; i++) {
		// 	Sleep(200);
		// 	to->write(buffer(e, (byte *)"a", 1));
		// }

		for (nat i = 0; i < ARRAY_COUNT(utf8Data2); i++) {
			Sleep(200);
			// std::wcout << (char)utf8Data2[i] << std::flush;
			to->write(buffer(e, utf8Data2 + i, 1));
		}

		to->write(buffer(e, (byte *)"Good bye!", 9));
	}

}
