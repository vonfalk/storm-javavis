#pragma once

#include "Shared/Io/Stream.h"

namespace graphics {

	typedef ::IStream ComStream;

	// A wrapper around storm::IStream to support the com object IStream.
	// It assumes that the Stream will exist the entire lifetime of the StreamWrapper object.
	class StreamWrapper : public ComStream {
		StreamWrapper(Par<storm::IStream> src) : src(src->randomAccess()) {
			refcount = 1;
		}

		~StreamWrapper() {}

	public:
		static StreamWrapper *create(Par<storm::IStream> src) {
			return new StreamWrapper(src);
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) {
			if (iid == __uuidof(IUnknown) || iid == __uuidof(ComStream) || iid == __uuidof(ISequentialStream)) {
				*ppvObject = static_cast<ComStream*>(this);
				AddRef();
				return S_OK;
			} else {
				return E_NOINTERFACE;
			}
		}

		virtual ULONG STDMETHODCALLTYPE AddRef(void) {
			return (ULONG)InterlockedIncrement(&refcount);
		}

		virtual ULONG STDMETHODCALLTYPE Release(void) {
			ULONG res = (ULONG)InterlockedDecrement(&refcount);
			if (res == 0)
				delete this;
			return res;
		}

		// ISequentialStream Interface
	public:
		virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead) {
			*pcbRead = ULONG(src->read(Buffer(pv, nat(cb))));
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten) {
			return E_NOTIMPL;
			// src.write(cb, pv);
			// *pcbWritten = cb;
			// return S_OK;
		}

		// IStream Interface
	public:
		virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE CopyTo(ComStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE Commit(DWORD) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE Revert(void) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE Clone(ComStream **) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer) {
			int64 p = liDistanceToMove.QuadPart;

			switch (dwOrigin) {
			case STREAM_SEEK_SET:
				src->seek(Word(p));
				break;
			case STREAM_SEEK_CUR:
				src->seek(Word(src->tell() + p));
				break;
			case STREAM_SEEK_END:
				src->seek(Word(src->length() + p));
				break;
			default:
				return STG_E_INVALIDFUNCTION;
			}

			if (lpNewFilePointer)
				lpNewFilePointer->QuadPart = src->tell();
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag) {
			pStatstg->cbSize.QuadPart = src->length();
			return S_OK;
		}

	private:
		LONG refcount;
		Auto<storm::RIStream> src;
	};

}
