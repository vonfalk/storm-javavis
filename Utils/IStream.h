#pragma once

#include "Stream.h"

namespace util {

	// A wrapper around util::Stream to support the com object IStream.
	// It assumes that the Stream will exist the entire lifetime of the StreamWrapper object.
	class StreamWrapper : public IStream {
		StreamWrapper(Stream &src) : src(src) {
			refcount = 1;
		}

		~StreamWrapper() {}
	public:
		static StreamWrapper *create(Stream &src) {
			return new StreamWrapper(src);
		}

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
		{ 
			if (iid == __uuidof(IUnknown) || iid == __uuidof(IStream) || iid == __uuidof(ISequentialStream)) {
				*ppvObject = static_cast<IStream*>(this);
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
			ULONG res = (ULONG) InterlockedDecrement(&refcount);
			if (res == 0) delete this;
			return res;
		}

		// ISequentialStream Interface
	public:
		virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead) {
			*pcbRead = ULONG(src.read(nat(cb), pv));
			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten) {
			src.write(cb, pv);
			*pcbWritten = cb;
			return S_OK;
		}

		// IStream Interface
	public:
		virtual HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) { 
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) {
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

		virtual HRESULT STDMETHODCALLTYPE Clone(IStream **) {
			return E_NOTIMPL;
		}

		virtual HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER liDistanceToMove, DWORD dwOrigin, ULARGE_INTEGER* lpNewFilePointer) {
			switch(dwOrigin) {
				case STREAM_SEEK_SET:
					src.seek(nat(liDistanceToMove.QuadPart));
					break;
				case STREAM_SEEK_CUR:
					src.seek(nat(liDistanceToMove.QuadPart + src.pos()));
					break;
				case STREAM_SEEK_END:
					src.seek(nat(liDistanceToMove.QuadPart - src.length()));
					break;
				default:
					return STG_E_INVALIDFUNCTION;
					break;
			}

			return S_OK;
		}

		virtual HRESULT STDMETHODCALLTYPE Stat(STATSTG* pStatstg, DWORD grfStatFlag) {
			pStatstg->cbSize.QuadPart = src.length();
			return S_OK;
		}

	private:
		LONG refcount;
		Stream &src;
	};

}