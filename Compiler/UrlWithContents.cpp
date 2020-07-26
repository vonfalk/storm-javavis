#include "stdafx.h"
#include "UrlWithContents.h"
#include "Core/Io/MemStream.h"
#include "Core/Io/Utf8Text.h"

namespace storm {

	UrlWithContents::UrlWithContents(Url *src, Buffer data) : Url(*src), data(data) {}

	UrlWithContents::UrlWithContents(Url *src, Str *data) : Url(*src) {
		MemOStream *out = new (this) MemOStream();
		TextOutput *text = new (this) Utf8Output(out);
		text->write(data);
		text->flush();
		this->data = out->buffer();
	}

	IStream *UrlWithContents::read() {
		return new (this) MemIStream(data);
	}

}
