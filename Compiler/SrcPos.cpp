#include "stdafx.h"
#include "SrcPos.h"
#include "Core/CloneEnv.h"
#include "Core/StrBuf.h"

namespace storm {

	SrcPos::SrcPos() : file(null), pos(0) {}

	SrcPos::SrcPos(Url *file, Nat pos) : file(file), pos(pos) {}

	Bool SrcPos::unknown() const {
		return file == null;
	}

	SrcPos SrcPos::operator +(Nat v) const {
		return SrcPos(file, pos + v);
	}

	void SrcPos::deepCopy(CloneEnv *env) {
		cloned(file, env);
	}

	Bool SrcPos::operator ==(SrcPos o) const {
		if (!file && !o.file)
			return true;
		if (!file || !o.file)
			return false;
		return *file == *o.file
			&& pos == o.pos;
	}

	Bool SrcPos::operator !=(SrcPos o) const {
		return !(*this == o);
	}

	wostream &operator <<(wostream &to, const SrcPos &p) {
		if (p.unknown())
			return to << L"<unknown location>";
		else
			return to << p.file << L"(" << p.pos << L")";
	}

	StrBuf &operator <<(StrBuf &to, SrcPos p) {
		if (p.unknown())
			return to << L"<unknown location>";
		else
			return to << p.file << L"(" << p.pos << L")";
	}

}
