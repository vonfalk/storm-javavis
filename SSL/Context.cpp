#include "stdafx.h"
#include "Context.h"
#include "Core/Exception.h"

namespace ssl {

	Context::Context() : created(null) {}

	Context::Context(const Context &o) : created(o.created) {
		if (created)
			created->ref();
	}

	Context::~Context() {
		if (created)
			created->unref();
	}

	void Context::deepCopy(CloneEnv *) {
		// No need to deep copy at the moment. We don't modify objects once they are in here.
	}

	SSLContext *Context::data() {
		if (!created)
			created = createData();
		return created;
	}

	void Context::invalidate() {
		if (created)
			created->unref();
		created = null;
	}

	SSLContext *Context::createData() {
		throw new (this) NotSupported(S("Must use one of the subclasses to Context provided by the SSL library."));
	}

}
