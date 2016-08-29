#include "stdafx.h"
#include "Transform.h"
#include "Listing.h"

namespace code {

	Transform::Transform() {}

	void Transform::before(Listing *dest, Listing *src) {}

	void Transform::during(Listing *dest, Listing *src, Nat line) {}

	void Transform::after(Listing *dest, Listing *src) {}

	Listing *transform(Listing *src, Transform *use) {
		Listing *result = src->createShell();

		use->before(result, src);

		for (Nat i = 0; i < src->count(); i++) {
			if (Array<Label> *labels = src->labels(i)) {
				for (Nat j = 0; j < labels->count(); j++)
					*result << labels->at(j);
			}

			use->during(result, src, i);
		}

		use->after(result, src);

		return result;
	}

}
