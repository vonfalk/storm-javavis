#include "stdafx.h"
#include "Binary.h"

namespace code {

	Binary::Binary(Arena *arena, Listing *listing) : arena(arena), code(null) {
		Listing *tfm = arena->transform(listing, this);
		PVAR(tfm);

		LabelOutput *labels = arena->labelOutput();
		arena->output(tfm, labels);

		Output *output = arena->codeOutput(labels->offsets, labels->tell());
		arena->output(tfm, output);

		// TODO: Find the allocated memory from 'output'.
	}

}
