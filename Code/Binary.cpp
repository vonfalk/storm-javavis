#include "stdafx.h"
#include "Binary.h"

namespace code {

	Binary::Binary(Arena *arena, Listing *listing) {
		Listing *tfm = arena->transform(listing, this);
		// PVAR(tfm);

		LabelOutput *labels = arena->labelOutput();
		arena->output(tfm, labels);

		CodeOutput *output = arena->codeOutput(this, labels);
		arena->output(tfm, output);

		set(output->codePtr(), output->tell());
	}

}
