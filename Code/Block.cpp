#include "stdafx.h"
#include "Block.h"
#include "Core/StrBuf.h"

namespace code {

	Block::Block() : id(-1) {}

	Block::Block(Nat id) : id(id) {}

	void Block::deepCopy(CloneEnv *) {}

	wostream &operator <<(wostream &to, Block l) {
		if (l.id == Block().id)
			return to << L"invalid block";
		return to << L"Block" << l.id;
	}

	StrBuf &operator <<(StrBuf &to, Block l) {
		if (l.id == Block().id)
			return to << S("invalid block");
		return to << S("Block") << l.id;
	}

}
