#include "stdafx.h"
#include "Block.h"
#include "Core/StrBuf.h"

namespace code {

	Block::Block() : id(-1) {}

	Block::Block(Nat id) : id(id) {}

	wostream &operator <<(wostream &to, Block l) {
		if (l.id == Block().id)
			return to << L"invalid part";
		return to << L"Part:" << l.id;
	}

	StrBuf &operator <<(StrBuf &to, Block l) {
		if (l.id == Block().id)
			return to << L"invalid part";
		return to << L"Part:" << l.id;
	}

	Part::Part() : id(-1) {}

	Part::Part(Nat id) : id(id) {}

	Part::Part(Block b) : id(b.id) {}

	wostream &operator <<(wostream &to, Part l) {
		if (l.id == -1)
			return to << L"invalid part";
		return to << L"Part:" << l.id;
	}

	StrBuf &operator <<(StrBuf &to, Part l) {
		if (l.id == -1)
			return to << L"invalid part";
		return to << L"Part:" << l.id;
	}

}
