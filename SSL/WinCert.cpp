#include "stdafx.h"
#include "WinCert.h"
#include "Exception.h"

#ifdef WINDOWS

namespace ssl {

	WinSSLCert *WinSSLCert::fromPEM(Str *data) {
		return new WinSSLCert();
	}

	WinSSLCert *WinSSLCert::windows() {
		ref();
		return this;
	}

	OpenSSLCert *WinSSLCert::openSSL() {
		throw new (runtime::someEngine()) SSLError(S("OpenSSL is not supported on Windows."));
	}

	void WinSSLCert::output(StrBuf *to) {
		*to << S("<SSL certificate, windows>");
	}

}

#endif
