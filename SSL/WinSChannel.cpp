#include "stdafx.h"
#include "WinSChannel.h"
#include "Exception.h"

#ifdef WINDOWS

namespace ssl {

	// Inspiration for this implementation:
	// Racket: https://github.com/racket/racket/blob/master/racket/collects/net/win32-ssl.rkt
	// MSDN: https://docs.microsoft.com/en-us/windows/win32/secauthn/creating-a-secure-connection-using-schannel

	// Note: We could use InitSecurityInterface and use the function table returned from there. I'm
	// not sure what is the main benefit of that though.

	SChannelContext::SChannelContext(CredHandle creds) : credentials(creds) {}

	SChannelContext::~SChannelContext() {
		FreeCredentialsHandle(&credentials);
	}

	SChannelContext *SChannelContext::createClient() {
		SECURITY_STATUS status;
		CredHandle credentials;
		TimeStamp expires;

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
		status = AcquireCredentialsHandle(NULL, SCHANNEL_NAME, SECPKG_CRED_OUTBOUND,
										NULL, &data, NULL, NULL,
										&credentials, &expires);

		if (status == SEC_E_SECPKG_NOT_FOUND)
			throw new (runtime::someEngine()) SSLError(S("Failed to find the SChannel security package."));
		else if (status != SEC_E_OK)
			throw new (runtime::someEngine()) SSLError(S("Failed to acquire credentials for SChannel."));

		return new SChannelContext(credentials);
	}


	SChannelSession::SChannelSession(SChannelContext *data, const String &host) : data(data), host(host) {
		data->ref();
		SecInvalidateHandle(&context);
	}

	SChannelSession::~SChannelSession() {
		data->unref();
		if (SecIsValidHandle(&context))
			DeleteSecurityContext(&context);
	}

	int SChannelSession::initSession(Engine &e, Buffer &inBuffer, Buffer &outBuffer) {
		SECURITY_STATUS status;
		// Ask it to allocate output buffers for us. That means we don't have to worry about the size of them.
		ULONG requirements = ISC_REQ_ALLOCATE_MEMORY;
		// Some more good features.
		requirements |= ISC_REQ_REPLAY_DETECT | ISC_REQ_SEQUENCE_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM;
		ULONG attrsOut = 0;
		CtxtHandle *currentHandle = null;
		SecBufferDesc *inputParam = null;

		SecBuffer inBuffers[2] = {
			{ 0, SECBUFFER_TOKEN, null },
			{ 0, SECBUFFER_EMPTY, null }
		};
		SecBufferDesc input = {
			SECBUFFER_VERSION, 2, inBuffers
		};

		if (!SecIsValidHandle(&context)) {
			PLN(L"First call");
			// First call.
			currentHandle = null;
			inputParam = null;
		} else {
			PLN(L"Subsequent call");
			// Other calls.
			currentHandle = &context;
			inputParam = &input;
			inBuffers[0].cbBuffer = inBuffer.filled();
			inBuffers[0].pvBuffer = inBuffer.dataPtr();
		}

		// Note: Doc says we need a SECBUFFER_ALERT, but it works anyway.
		SecBuffer outBuffers[1] = { { 0, SECBUFFER_EMPTY, null } };
		SecBufferDesc output = {
			SECBUFFER_VERSION, 1, outBuffers,
		};

		status = InitializeSecurityContext(&data->credentials, currentHandle,
										(SEC_WCHAR *)host.c_str(), requirements,
										0 /* reserved */, 0 /* not used for schannel */,
										inputParam, 0 /* reserved */,
										&context, &output,
										&attrsOut, NULL /* expires */);

		// See if there is any unconsumed data in the input buffer.
		if (input.pBuffers[1].BufferType == SECBUFFER_EXTRA) {
			// Shift data back and keep unconsumed data in the beginning of "input".
			PVAR(input.pBuffers[1].cbBuffer);
			inBuffer.shift(inBuffer.filled() - input.pBuffers[1].cbBuffer);
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// It was incomplete, just don't clear the buffer.
		} else {
			// No unconsumed data. Clear it.
			inBuffer.filled(0);
		}

		int result = 0;
		bool error = false;
		if (status == SEC_I_CONTINUE_NEEDED) {
			// Send to server!
			SecBuffer b = output.pBuffers[0];
			if (outBuffer.count() < b.cbBuffer)
				outBuffer = storm::buffer(e, b.cbBuffer);

			memcpy(outBuffer.dataPtr(), b.pvBuffer, b.cbBuffer);
			outBuffer.filled(b.cbBuffer);

			// Not done. More messages.
			result = -1;

			PLN(L"Continue");
		} else if (status == SEC_E_OK) {
			// All done!
			result = 0;
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			if (input.pBuffers[1].BufferType == SECBUFFER_MISSING) {
				result = input.pBuffers[1].cbBuffer;
			} else {
				// Just provide a decent guess if we cant find a guess from the API.
				result = 32;
			}
			PLN(L"NOT ENOUGH");
		} else if (status == SEC_E_INVALID_TOKEN) {
			PLN(L"INV TOKEN");
			error = true;
		} else {
			PLN(L"ERROR");
			error = true;
		}

		// Clean up.
		if (output.pBuffers[0].pvBuffer)
			FreeContextBuffer(output.pBuffers[0].pvBuffer);

		if (error) {
			PVAR(SEC_I_COMPLETE_AND_CONTINUE);
			PVAR(SEC_I_COMPLETE_NEEDED);
			PVAR(SEC_I_CONTINUE_NEEDED);
			PVAR(SEC_E_INCOMPLETE_MESSAGE);
			PVAR(SEC_I_INCOMPLETE_CREDENTIALS);
			PVAR(SEC_E_INSUFFICIENT_MEMORY);
			PVAR(SEC_E_INTERNAL_ERROR);
			PVAR(SEC_E_INVALID_HANDLE);
			PVAR(SEC_E_INVALID_TOKEN);
			PVAR(SEC_E_LOGON_DENIED);
			PVAR(SEC_E_NO_AUTHENTICATING_AUTHORITY);
			PVAR(SEC_E_NO_CREDENTIALS);
			PVAR(SEC_E_TARGET_UNKNOWN);
			PVAR(SEC_E_UNSUPPORTED_FUNCTION);
			PVAR(SEC_E_WRONG_PRINCIPAL);
			PVAR(SEC_E_ILLEGAL_MESSAGE);
			throw new (e) SSLError(TO_S(e, S("Failed to initialize an SSL session: ") << status));
		}

		return result;
	}

}

#endif
