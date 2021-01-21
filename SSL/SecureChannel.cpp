#include "stdafx.h"
#include "SecureChannel.h"
#include "Exception.h"

#ifdef WINDOWS

namespace ssl {

	// Inspiration for this implementation:
	// Racket: https://github.com/racket/racket/blob/master/racket/collects/net/win32-ssl.rkt
	// MSDN: https://docs.microsoft.com/en-us/windows/win32/secauthn/creating-a-secure-connection-using-schannel

	// Note: We could use InitSecurityInterface and use the function table returned from there. I'm
	// not sure what is the main benefit of that though.

#define SET_ERROR(MSG) case MSG: msg = WIDEN(#MSG); break

	static void throwError(Engine &e, const wchar *error, SECURITY_STATUS status) {
		const wchar *msg = null;
		switch (status) {
			SET_ERROR(SEC_I_COMPLETE_AND_CONTINUE);
			SET_ERROR(SEC_I_COMPLETE_NEEDED);
			SET_ERROR(SEC_I_CONTINUE_NEEDED);
			SET_ERROR(SEC_I_CONTEXT_EXPIRED);
			SET_ERROR(SEC_E_CONTEXT_EXPIRED);
			SET_ERROR(SEC_E_INCOMPLETE_MESSAGE);
			SET_ERROR(SEC_I_INCOMPLETE_CREDENTIALS);
			SET_ERROR(SEC_E_INSUFFICIENT_MEMORY);
			SET_ERROR(SEC_E_INTERNAL_ERROR);
			SET_ERROR(SEC_E_INVALID_HANDLE);
			SET_ERROR(SEC_E_INVALID_TOKEN);
			SET_ERROR(SEC_E_LOGON_DENIED);
			SET_ERROR(SEC_E_NO_AUTHENTICATING_AUTHORITY);
			SET_ERROR(SEC_E_NO_CREDENTIALS);
			SET_ERROR(SEC_E_TARGET_UNKNOWN);
			SET_ERROR(SEC_E_UNSUPPORTED_FUNCTION);
			SET_ERROR(SEC_E_WRONG_PRINCIPAL);
			SET_ERROR(SEC_E_ILLEGAL_MESSAGE);
		}

		if (msg)
			throw new (e) SSLError(TO_S(e, error << msg));
		else
			throw new (e) SSLError(TO_S(e, error << status));
	}

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


	SChannelSession::SChannelSession(SChannelContext *data)
		: data(data), maxMessageSize(0), blockSize(0) {

		data->ref();
		SecInvalidateHandle(&context);
	}

	SChannelSession::~SChannelSession() {
		data->unref();
		if (SecIsValidHandle(&context))
			DeleteSecurityContext(&context);
	}

	int SChannelSession::initSession(Engine &e, Buffer &inBuffer, Buffer &outBuffer, const wchar *host) {
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
			// First call.
			currentHandle = null;
			inputParam = null;
		} else {
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
										(SEC_WCHAR *)host, requirements,
										0 /* reserved */, 0 /* not used for schannel */,
										inputParam, 0 /* reserved */,
										&context, &output,
										&attrsOut, NULL /* expires */);

		// See if there is any unconsumed data in the input buffer.
		if (input.pBuffers[1].BufferType == SECBUFFER_EXTRA) {
			// Shift data back and keep unconsumed data in the beginning of "input".
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
		} else if (status == SEC_E_OK) {
			// All done, set the maximum size for encryption/decryption.
			initSizes();
			result = 0;
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			if (input.pBuffers[1].BufferType == SECBUFFER_MISSING) {
				result = input.pBuffers[1].cbBuffer;
			} else {
				// Just provide a decent guess if we cant find a guess from the API.
				result = 32;
			}
		} else {
			error = true;
		}

		// Clean up.
		if (output.pBuffers[0].pvBuffer)
			FreeContextBuffer(output.pBuffers[0].pvBuffer);

		if (error)
			throwError(e, S("Failed to initialize an SSL session: "), status);

		return result;
	}

	void SChannelSession::encrypt(Engine &e, const Buffer &input, Nat offset, Buffer &output) {
		// "reserve" size for header and trailer.
		Nat inputSize = input.filled() - offset;
		Nat size = headerSize + trailerSize + inputSize;
		if (output.count() < size)
			output = buffer(e, size);

		output.filled(size);
		memcpy(output.dataPtr() + headerSize, input.dataPtr() + offset, inputSize);

		SecBuffer buffers[4] = {
			{ headerSize, SECBUFFER_STREAM_HEADER, output.dataPtr() },
			{ inputSize, SECBUFFER_DATA, output.dataPtr() + headerSize },
			{ trailerSize, SECBUFFER_STREAM_TRAILER, output.dataPtr() + headerSize + inputSize },
			{ 0, SECBUFFER_EMPTY, null },
		};
		SecBufferDesc bufferDesc = {
			SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
		};
		SECURITY_STATUS status = EncryptMessage(&context, 0, &bufferDesc, 0);
		if (status != SEC_E_OK)
			throwError(e, S("Failed to encrypt a message: "), status);
	}

	static SecBuffer *findBuffer(SecBufferDesc &in, unsigned long type) {
		for (unsigned long i = 0; i < in.cBuffers; i++) {
			if (in.pBuffers[i].BufferType == type)
				return &in.pBuffers[i];
		}
		return null;
	}

	SChannelSession::Markers SChannelSession::decrypt(Engine &e, Buffer &data, Nat offset) {
		SecBuffer buffers[4] = {
			{ data.filled() - offset, SECBUFFER_DATA, data.dataPtr() + offset },
			{ 0, SECBUFFER_EMPTY, null },
			{ 0, SECBUFFER_EMPTY, null },
			{ 0, SECBUFFER_EMPTY, null },
		};
		SecBufferDesc bufferDesc = {
			SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
		};
		SECURITY_STATUS status = DecryptMessage(&context, &bufferDesc, 0, NULL);

		Markers m = { offset, offset, offset, false };

		if (status == SEC_E_OK) {
			{
				SecBuffer *plaintext = findBuffer(bufferDesc, SECBUFFER_DATA);
				m.plaintextStart = (byte *)plaintext->pvBuffer - data.dataPtr();
				m.plaintextEnd = m.plaintextStart + plaintext->cbBuffer;
			}

			if (SecBuffer *ciphertext = findBuffer(bufferDesc, SECBUFFER_EXTRA)) {
				m.remainingStart = (byte *)ciphertext->pvBuffer - data.dataPtr();
			} else {
				m.remainingStart = data.filled();
			}
		} else if (status == SEC_E_INCOMPLETE_MESSAGE) {
			// Tell the caller that they need to get more data.
			// We don't need to do anything here due to the initialization above.
		} else if (status == SEC_I_CONTEXT_EXPIRED) {
			// The remote end shut the connection down.
			m.shutdown = true;
		} else {
			throwError(e, S("Failed to decrypt a message: "), status);
		}

		return m;
	}

	Buffer SChannelSession::shutdown(Engine &e) {
		{
			DWORD token = SCHANNEL_SHUTDOWN;
			SecBuffer buffers[1] = {
				{ sizeof(DWORD), SECBUFFER_TOKEN, &token }
			};
			SecBufferDesc bufferDesc = {
				SECBUFFER_VERSION, ARRAY_COUNT(buffers), buffers
			};
			SECURITY_STATUS status = ApplyControlToken(&context, &bufferDesc);
			if (status != SEC_E_OK)
				throwError(e, S("Failed to shutdown: "), status);
		}

		// Now, we just need to call initSession(?) to get what to reply!
		// TODO: We should call initSession for server/client as appropriate.
		// TODO: Doc says we might need to loop this. Based on the example
		// from http://www.coastrd.com/c-schannel-smtp, it seems unlikely for SChannel.
		Buffer input;
		Buffer output;
		int ok = initSession(e, input, output, null);
		if (ok != 0)
			WARNING(L"We might need to do something differently!");
		return output;
	}

	void SChannelSession::initSizes() {
		SecPkgContext_StreamSizes ss;
		QueryContextAttributes(&context, SECPKG_ATTR_STREAM_SIZES, &ss);
		maxMessageSize = ss.cbMaximumMessage;
		blockSize = ss.cbBlockSize;
		headerSize = ss.cbHeader;
		trailerSize = ss.cbTrailer;
	}

}

#endif
