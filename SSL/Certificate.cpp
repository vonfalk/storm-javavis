#include "stdafx.h"
#include "Certificate.h"
#include "WinCert.h"
#include "Core/Io/Text.h"

namespace ssl {

	Certificate *Certificate::loadPEM(Str *data) {
		SSLCert *c = null;
#ifdef WINDOWS
		c = WinSSLCert::fromPEM(data);
#else
		c = OpenSSLCert::fromPEM(data);
#endif
		return new (data) Certificate(c);
	}

	Certificate *Certificate::loadPEM(Url *file) {
		return loadPEM(readAllText(file));
	}

	Certificate::Certificate(SSLCert *data) : data(data) {}

	Certificate::Certificate(const Certificate &o) : data(o.data) {
		if (data)
			data->ref();
	}

	Certificate::~Certificate() {
		if (data)
			data->unref();
		data = null;
	}

	void Certificate::toS(StrBuf *to) const {
		if (data)
			data->output(to);
		else
			*to << S("<no certificate>");
	}

}
