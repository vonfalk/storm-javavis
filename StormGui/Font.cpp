#include "stdafx.h"
#include "Font.h"

namespace stormgui {

	/**
	 * Shared data between font objects. Not cloned between different threads, and is therefore
	 * protected by explicit locks and atomics. Be careful.
	 * It generally assumes that once something is created, it will not be destroyed again, unless we're
	 * the only reference to an object.
	 */
	struct FontData {
		// Create.
		FontData() : refs(1) {
			hFont = (HFONT)INVALID_HANDLE_VALUE;
		}

		// Destroy.
		~FontData() {
			clear();
		}

		// Refcount.
		nat refs;

		// Increase
		void addRef() {
			atomicIncrement(refs);
		}

		// Decrease.
		void release() {
			if (atomicDecrement(refs) == 0)
				delete this;
		}

		// Invalidate. Returns either this object or a new one, where no data is created.
		FontData *invalidate() {
			if (atomicDecrement(refs) == 0) {
				// We're alone, clear.
				clear();
				addRef();
				return this;
			} else {
				return new FontData();
			}
		}

		// Clear data. _not_ thread safe, only to be used inside 'invalidate'.
		void clear() {
			if (hFont != INVALID_HANDLE_VALUE)
				DeleteObject(hFont);
			hFont = (HFONT)INVALID_HANDLE_VALUE;
		}

		// WIN32 font.
		HFONT hFont;

		// Lock for modifying any members.
		os::Lock lock;
	};

	Font::Font(Par<Str> face, Float height) {
		fName = face->v;
		fHeight = height;
		fWeight = FW_NORMAL;
		fItalic = false;
		fUnderline = false;
		fStrikeOut = false;
		shared = new FontData();
	}

	Font::Font(Par<Font> f) {
		fName = f->fName;
		fHeight = f->fHeight;
		fWeight = f->fWeight;
		fItalic = f->fItalic;
		fUnderline = f->fUnderline;
		fStrikeOut = f->fStrikeOut;
		shared = f->shared;
		shared->addRef();
	}

	Font::Font(LOGFONT &f) {
		fName = f.lfFaceName;
		fHeight = (float)f.lfHeight;
		fWeight = (int)f.lfWeight;
		fItalic = f.lfItalic == TRUE;
		fUnderline = f.lfUnderline == TRUE;
		fStrikeOut = f.lfStrikeOut == TRUE;
		shared = new FontData();
	}

	Font::~Font() {
		shared->release();
	}

	void Font::name(Par<Str> name) {
		fName = name->v;
		changed();
	}

	void Font::height(Float h) {
		fHeight = h;
		changed();
	}

	void Font::weight(Int w) {
		fWeight = w;
		changed();
	}

	void Font::italic(Bool u) {
		fItalic = u;
		changed();
	}

	void Font::underline(Bool u) {
		fUnderline = u;
		changed();
	}

	void Font::strikeOut(Bool u) {
		fStrikeOut = u;
		changed();
	}

	void Font::changed() {
		shared = shared->invalidate();
	}

	HFONT Font::handle() {
		os::Lock::L z(shared->lock);
		if (shared->hFont == INVALID_HANDLE_VALUE) {
			LOGFONT lf;
			zeroMem(lf);
			lf.lfHeight = (LONG)fHeight;
			lf.lfWidth = 0;
			lf.lfEscapement = 0;
			lf.lfOrientation = 0;
			lf.lfWeight = fWeight;
			lf.lfItalic = fItalic ? TRUE : FALSE;
			lf.lfUnderline = fUnderline ? TRUE : FALSE;
			lf.lfStrikeOut = fStrikeOut ? TRUE : FALSE;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
			lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			lf.lfQuality = DEFAULT_QUALITY;
			lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
			memcpy(lf.lfFaceName, fName.c_str(), max(sizeof(lf.lfFaceName), fName.size() * sizeof(wchar)));
			shared->hFont = CreateFontIndirect(&lf);
		}
		return shared->hFont;
	}

	Font *defaultFont(EnginePtr e) {
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm) - sizeof(ncm.iPaddedBorderWidth);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		return CREATE(Font, e.v, ncm.lfMessageFont);
	}

}
