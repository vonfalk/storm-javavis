#include "stdafx.h"
#include "WinCert.h"

#ifdef WINDOWS

#include "WinErrorMsg.h"
#include "Exception.h"
#include "Core/Io/Text.h"
#pragma comment (lib, "crypt32.lib")
#pragma comment (lib, "advapi32.lib")

namespace ssl {

	static std::vector<BYTE> decodeBase64(Str *data) {
		DWORD size = 0;
		if (!CryptStringToBinaryW(data->c_str(), NULL, CRYPT_STRING_BASE64_ANY, NULL, &size, NULL, NULL))
			throwError(data->engine(), S("Failed to decode PEM data: "), GetLastError());

		std::vector<BYTE> result(size, 0);
		if (!CryptStringToBinaryW(data->c_str(), NULL, CRYPT_STRING_BASE64_ANY, &result[0], &size, NULL, NULL))
			throwError(data->engine(), S("Failed to decode PEM data: "), GetLastError());

		return result;
	}

	WinSSLCert *WinSSLCert::fromPEM(Str *data) {
		std::vector<BYTE> raw(decodeBase64(data));

		const CERT_CONTEXT *c = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, &raw[0], raw.size());
		if (!c)
			throwError(data->engine(), S("Failed to decode certificate data: "), GetLastError());

		return new WinSSLCert(c);
	}

	WinSSLCert::WinSSLCert(const CERT_CONTEXT *c) : data(c) {}

	WinSSLCert::~WinSSLCert() {
		CertFreeCertificateContext(data);
	}

	WinSSLCert *WinSSLCert::windows() {
		ref();
		return this;
	}

	OpenSSLCert *WinSSLCert::openSSL() {
		throw new (runtime::someEngine()) SSLError(S("OpenSSL is not supported on Windows."));
	}

	static GcArray<wchar> *decodeName(Engine &e, CERT_NAME_BLOB &blob, DWORD stringType) {
		DWORD size = CertNameToStrW(X509_ASN_ENCODING, &blob, stringType, NULL, 0);

		GcArray<wchar> *result = runtime::allocArray<wchar>(e, &wcharArrayType, size);
		CertNameToStrW(X509_ASN_ENCODING, &blob, stringType, result->v, size);
		return result;
	}

	void WinSSLCert::output(StrBuf *to) {
		CERT_INFO *info = data->pCertInfo;
		*to << S("Subject: ") << decodeName(to->engine(), info->Subject, CERT_X500_NAME_STR)->v;
		*to << S(", issuer: ") << decodeName(to->engine(), info->Issuer, CERT_X500_NAME_STR)->v;
	}


	WinSSLCertKey *WinSSLCertKey::fromPEM(Str *data) {
		std::vector<BYTE> raw(decodeBase64(data));

		DWORD size = 0;
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY,
									&raw[0], raw.size(), 0, NULL, NULL, &size))
			throwError(data->engine(), S("Failed to decode the key: "), GetLastError());

		std::vector<BYTE> decoded(size, 0);
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY,
									&raw[0], raw.size(), 0, NULL, &decoded[0], &size))
			throwError(data->engine(), S("Failed to decode the key: "), GetLastError());

		return new WinSSLCertKey(decoded);
	}

	WinSSLCertKey::WinSSLCertKey(const std::vector<byte> &data) : data(data) {}

	WinSSLCertKey::~WinSSLCertKey() {}

	bool WinSSLCertKey::validate(Engine &e, SSLCert *cert) {
		RefPtr<WinSSLCert> c = cert->windows();

		// Try to import it into a temporary store and associate it with the certificate?
		TODO(L"Implement early validation");

		return true;
	}

	WinSSLCertKey *WinSSLCertKey::windows() {
		ref();
		return this;
	}

	OpenSSLCertKey *WinSSLCertKey::openSSL() {
		// This is probably fairly easy to do, but useless since we can not do it for certificates at the moment.
		throw new (runtime::someEngine()) SSLError(S("OpenSSL is not supported on Windows."));
	}


	// Test code for loading a certificate using the Windows API.
#if 0
	// Inspired partially from: http://www.idrix.fr/Root/Samples/capi_pem.cpp
	// linked from: https://stackoverflow.com/questions/8412838/how-to-import-private-key-in-pem-format-using-wincrypt-and-c
	static const CERT_CONTEXT *loadCert() {
		// Hint:
		// CryptDecodeObjectEx might be used to decrypt PEM, then
		// CryptImportKey
		// Perhaps also look at PFXImportCertStore

		Engine &e = runtime::someEngine();

		Str *certData = readAllText(cwdUrl(e)->push(new (e) Str(S("..")))->push(new (e) Str(S("cert.pem"))));
		Str *keyData = readAllText(cwdUrl(e)->push(new (e) Str(S("..")))->push(new (e) Str(S("key.pem"))));

		// Note: The flag CRYPT_STRING_ANY might be interesting, that allows binary copies as well!
		// Note: Last parameter can give us encoding actually used.
		DWORD certSize = 0;
		if (!CryptStringToBinaryW(certData->c_str(), NULL, CRYPT_STRING_BASE64_ANY, NULL, &certSize, NULL, NULL))
			PLN(L"FAIL1");
		BYTE *cert = new BYTE[certSize];
		if (!CryptStringToBinaryW(certData->c_str(), NULL, CRYPT_STRING_BASE64_ANY, cert, &certSize, NULL, NULL))
			PLN(L"FAIL2");

		DWORD keySize = 0;
		if (!CryptStringToBinaryW(keyData->c_str(), NULL, CRYPT_STRING_BASE64_ANY, NULL, &keySize, NULL, NULL))
			PLN(L"FAIL3");
		BYTE *key = new BYTE[keySize];
		if (!CryptStringToBinaryW(keyData->c_str(), NULL, CRYPT_STRING_BASE64_ANY, key, &keySize, NULL, NULL))
			PLN(L"FAIL4");


		// Load the certificate
		const CERT_CONTEXT *context = CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, cert, certSize);
		PVAR(context);
		PVAR(context->hCertStore);

		PLN(L"Loaded the certificate!");

		// CertFreeCertificateContext(context);

		// Load the key.
		// TODO: How to support encrypted keys?
		// We can decrypt with: openssl rsa -in <key> -out <key>
		DWORD rawKeySize = 0;
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, key, keySize, 0, NULL, NULL, &rawKeySize))
			PLN(L"Fail to load key1");
		BYTE *rawKey = new BYTE[rawKeySize];
		if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, key, keySize, 0, NULL, rawKey, &rawKeySize))
			PLN(L"Fail to load key2");

		// Note: This is deprecated. Is there a better way to do this? We have a certstore handle.
		// Note: Use NCryptImportKey instead. That's what the docs for CRYPT_KEY_PROV_INFO refers to as well.
		// Note: This works but is persistent in the system...
		// HCRYPTPROV provider = 0;
		// if (!CryptAcquireContext(&provider, S("TEMPNAME"), MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
		// 	DWORD error = GetLastError();
		// 	if (error == NTE_EXISTS) {
		// 		CryptAcquireContext(&provider, S("TEMPNAME"), MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
		// 		PVAR(GetLastError());
		// 	}

		// 	PVAR(error);
		// }
		// PVAR(provider);

		// HCRYPTKEY hKey = 0;
		// if (!CryptImportKey(provider, rawKey, rawKeySize, NULL, 0, &hKey)) {
		// 	DWORD error = GetLastError();
		// 	PLN(L"NO IMPORT KEY " << error);
		// }

		// CRYPT_KEY_PROV_INFO pkInfo = { 0 };
		// pkInfo.pwszContainerName = S("TEMPNAME");
		// pkInfo.pwszProvName = MS_ENHANCED_PROV;
		// pkInfo.dwProvType = PROV_RSA_SCHANNEL;
		// pkInfo.dwKeySpec = AT_KEYEXCHANGE;

		// CertSetCertificateContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID, 0, &pkInfo);

		// According to the documentation, setting a NULL name for the MS provider will generate a
		// unique container for each call. It is not persisted if we set the VERIFYCONTEXT
		// flag. Thus, if we keep a reference to it, SChannel will be able to access it for long enough.
		HCRYPTPROV provider = 0;
		if (!CryptAcquireContext(&provider, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, 0 & CRYPT_VERIFYCONTEXT)) {
		// if (!CryptAcquireContext(&provider, NULL, MS_DEF_RSA_SCHANNEL_PROV, PROV_RSA_SCHANNEL, 0 & CRYPT_VERIFYCONTEXT)) {
			DWORD error = GetLastError();
			PVAR(error);
		}
		PVAR(provider);

		HCRYPTKEY hKey = 0;
		if (!CryptImportKey(provider, rawKey, rawKeySize, NULL, 0, &hKey)) {
			DWORD error = GetLastError();
			PLN(L"NO IMPORT KEY " << error);
		}

		DWORD nameLen = 0;
		CryptGetProvParam(provider, PP_CONTAINER, NULL, &nameLen, 0);
		PVAR(GetLastError());
		BYTE *providerName = new BYTE[nameLen + 1];
		PVAR(nameLen);
		CryptGetProvParam(provider, PP_CONTAINER, providerName, &nameLen, 0);
		providerName[nameLen] = 0;
		PVAR((const char *)providerName);

		wchar *providerName16 = new wchar[nameLen + 1];
		PVAR(convert((const char *)providerName, providerName16, nameLen + 1));

		CRYPT_KEY_PROV_INFO pkInfo = { 0 };
		pkInfo.pwszContainerName = providerName16;
		pkInfo.pwszProvName = MS_DEF_RSA_SCHANNEL_PROV;
		pkInfo.dwProvType = PROV_RSA_SCHANNEL;
		pkInfo.dwKeySpec = AT_KEYEXCHANGE;
		pkInfo.dwFlags = CERT_SET_KEY_PROV_HANDLE_PROP_ID; // Use the key in the handle?

		// PVAR(CertSetCertificateContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID, 0, &pkInfo));

		// Nice, when we set this parameter, we get the same one from "AcquireCertificatePrivateKey"!
		// AcquireHandle for SChannel does not seem to care though...
		PVAR(CertSetCertificateContextProperty(context, CERT_KEY_PROV_HANDLE_PROP_ID, 0, (void *)provider)); // takes ownership of "provider".

		HCRYPTPROV nprov = 0;
		DWORD spec = 0;
		BOOL free = 0;
		PVAR(CryptAcquireCertificatePrivateKey(context, CRYPT_ACQUIRE_CACHE_FLAG, NULL, &nprov, &spec, &free));
		PVAR(nprov);

		BYTE encName[100];
		DWORD encNameSize;
		CertStrToNameW(X509_ASN_ENCODING, L"CN=test", CERT_X500_NAME_STR, NULL, encName, &encNameSize, NULL);
		PVAR(encNameSize);

		CERT_NAME_BLOB issuer = { encNameSize, encName };
		const CERT_CONTEXT *c = CertCreateSelfSignCertificate(provider, &issuer, 0, NULL, NULL, NULL, NULL, NULL);
		PVAR(c);

		// CERT_KEY_CONTEXT keyContext;
		// keyContext.cbSize = sizeof(keyContext);
		// keyContext.hCryptProv = provider;
		// keyContext.dwKeySpec = AT_KEYEXCHANGE;
		// CertSetCertificateContextProperty(context, CERT_KEY_CONTEXT_PROP_ID, 0, &keyContext);

		PLN(L"OK!");

		return context;


		// We still need to import the key. Maybe we can use: CryptImportKey for this?
		// See here: https://docs.microsoft.com/en-us/windows/win32/seccrypto/rsa-schannel-key-blobs

		// It should be possible to call DecodeObjectEx with the PKCS_RSA_PRIVATE_KEY parameter and then CryptImportKey.

		// There is a function called CryptImportPKCS8, but that is deprecated (and removed) in favor of PFXImportCertStore
		// Note: PFX seems to be (partially) synonymous with PKCS 12.
		// CRYPT_DATA_BLOB dataBlob = { bufSize, buffer };
		// DWORD flags = 0;
		// flags |= CRYPT_EXPORTABLE; // Allow exporting the keys?
		// flags |= PKCS12_NO_PERSIST_KEY; // Don't save the keys on disk.
		// HCERTSTORE store = PFXImportCertStore(&dataBlob, S("test"), flags);
		// DWORD error = GetLastError();
		// PVAR(store);
		// PVAR(error);

		// // Note: This is *probably* for the certificate. Both x509 and PKCS7 are for certs...
		// DWORD decodedCount = 0;
		// if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, PKCS_RSA_PRIVATE_KEY, buffer, bufSize, 0, NULL, NULL, &decodedCount)) {
		// 	PVAR(GetLastError());
		// }

		// PVAR(decodedCount);
		// // LocalFree(decoded);
	}
#endif

}

#endif
