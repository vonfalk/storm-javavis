#include "stdafx.h"
#include "CppDoc.h"
#include "Engine.h"
#include "Package.h"

namespace storm {

	CppDoc::CppDoc(Url *file, Nat entry, const CppParam *params) :
		data(file), entryInfo(entry << 1), params(params) {}

	CppDoc::CppDoc(const wchar *name, Nat entry, const CppParam *params) :
		data(name), entryInfo((entry << 1) | 0x1), params(params) {}

	const Url *CppDoc::file() {
		if (entryInfo & 0x1) {
			const wchar *name = (const wchar *)data;

			Package *root = engine().package();
			return root->url()->push(new (root) Str(name));
		} else {
			return (const Url *)data;
		}
	}

	Nat CppDoc::entry() {
		return entryInfo >> 1;
	}

	Doc *CppDoc::get() {
		assert(false, L"Implement me!");
		return null;
	}

}
