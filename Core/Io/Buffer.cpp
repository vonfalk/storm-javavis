#include "stdafx.h"
#include "Buffer.h"
#include "Core/CloneEnv.h"

namespace storm {

	static const GcType bufType = {
		GcType::tArray,
		null,
		null,
		sizeof(byte),
		0,
		{}
	};

	Buffer::Buffer() : data(null) {}

	Buffer::Buffer(GcArray<byte> *buf) : data(buf) {}

	Buffer::Buffer(const Buffer &o) : data(o.data) {}

	Buffer buffer(EnginePtr e, Nat count) {
		Buffer z(runtime::allocArray<byte>(e.v, &bufType, count));
		z.filled(count);
		return z;
	}

	void Buffer::deepCopy(CloneEnv *env) {
		if (data) {
			GcArray<byte> *n = runtime::allocArray<byte>(env->engine(), &bufType, data->count);
			n->filled = data->filled;
			for (nat i = 0; i < data->count; i++)
				n->v[i] = data->v[i];
			data = n;
		}
	}

}
