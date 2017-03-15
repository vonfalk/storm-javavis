#include "stdafx.h"
#include "Test.h"
#include "Read.h"
#include "Player.h"
#include "Core/Io/Url.h"

namespace sound {

	void testMe(EnginePtr e) {
		Url *base = cwdUrl(e)
			->push(new (e.v) Str(L".."))
			->push(new (e.v) Str(L"root"))
			->push(new (e.v) Str(L"res"))
			->push(new (e.v) Str(L"News Theme.ogg"));

		PVAR(base);
		Sound *stream = readSound(base);
		PVAR(stream);

		Player *p = new (e.v) Player(stream);
		p->play();
		p->wait();

		// Trigger a heap check...
		PLN(L"Checking...");
		for (Nat i = 0; i < 10000; i++)
			new (e.v) Str(L"A");
		PLN(L"Done!");
	}

}
