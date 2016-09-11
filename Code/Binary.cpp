#include "stdafx.h"
#include "Binary.h"

namespace code {

	Binary::Binary(Arena *arena, Listing *listing) : arena(arena), code(null) {
		Listing *tfm = arena->transform(listing, this);
		// PVAR(tfm);

		LabelOutput *labels = arena->labelOutput();
		arena->output(tfm, labels);

		CodeOutput *output = arena->codeOutput(labels);
		arena->output(tfm, output);

		code = output->codePtr();
	}

	void *Binary::rawPtr() const {
		return code;
	}

}
