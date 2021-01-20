#include "stdafx.h"
#include "Test.h"

namespace ssl {

	void test() {
		// Inspired by racket/collects/net/win32-ssl.rkt
		// Also here: https://docs.microsoft.com/en-us/windows/win32/secauthn/creating-a-secure-connection-using-schannel

		SecurityFunctionTable *table = InitSecurityInterface();
		PVAR(table);

		unsigned long num = 0;
		SecPkgInfo *packages;
		table->EnumerateSecurityPackages(&num, &packages);
		for (unsigned long i = 0; i < num; i++) {
			PVAR(packages[i].Name);
			PVAR(packages[i].Comment);
		}
		FreeContextBuffer(packages);

		SECURITY_STATUS status;

		// Only supported in Windows 10?
		// PVAR(SCHANNEL_CRED_VERSION);
		SCHANNEL_CRED data = {
			SCHANNEL_CRED_VERSION,
			0, // Number of private keys
			NULL, // Array of private keys
			NULL, // Root store
			0, // Reserved
			0, // Reserved
			0, // Number of algorithms to use (0 = system default)
			NULL, // Algorithms to use (0 = system default)
			0, // Protocols (we should set it to zero to let the system decide). This is where we specify TLS version.
			0, // Cipher strenght. 0 = system default
			0, // Session lifespan. 0 = default (10 hours)
			0, // Various flags. We might want to alter this later.
			0, // Kernel format. Should be zero.
		};

		// Acquire credentials.
		CredHandle credentials;
		TimeStamp expires;
		status = table->AcquireCredentialsHandle(NULL, SCHANNEL_NAME, SECPKG_CRED_OUTBOUND,
												NULL, &data, NULL, NULL,
												&credentials, &expires);

		PVAR(status);
		PVAR(SEC_E_OK);

		// Handle to create.
		// const wchar *server = L"fprg.se";
		// CtxtHandle context;
		// ULONG requirements = 0; // Required flags
		// ULONG attributes = 0; // received attributes
		// SecBufferDesc output = { ... };
		// status = table->InitializeSecurityContext(credentials, NULL, server, requirements,
		// 										0, 0, NULL /* input, null first time */, 0,
		// 										&context, &output, &attributes, NULL);

		// Status could be: SEC_I_COMPLETE_AND_CONTINUE, SEC_I_COMPLETE_NEEDED, SEC_I_CONTINUE_NEEDED
		// or SEC_E_OK if we are done.

		PLN(L"Working");
	}

}
